/*
 * A STREAMS module to turn off buffering in the Sun zs driver
 * using an internal interface provided for the Sun mouse driver.
 * Just push the module to turn buffering off, and pop it to turn
 * buffering on.  See <sys/tty.h> and <sundev/zsreg.h> for clues.
 * tkr@puffball.demon.co.uk 20apr93
 */

#include <sys/types.h>
#include <sys/stream.h>
#include <sys/param.h>
#include <sys/syslog.h>
#include <sys/tty.h>

static struct module_info minfo = {0, "zsunbuf", 0, INFPSZ, 0, 0};

static int zsunbufopen(), zsunbufrput(),
    zsunbufwput(), zsunbufclose();

static struct qinit rinit = {
    zsunbufrput, NULL, zsunbufopen, zsunbufclose, NULL, &minfo, NULL
};
static struct qinit winit = {
    zsunbufwput, NULL, NULL, NULL, NULL, &minfo, NULL
};
struct streamtab zsunbufinfo = { &rinit, &winit, NULL, NULL };

static int zsunbufopened = 0;

static int
zsunbufopen(q, dev, flag, sflag)
    queue_t *q;
    dev_t dev;
    int flag;
    int sflag;
{
#ifdef DEBUG
    log(LOG_INFO, "zsunbufopen:  set MC_SERVICEIMM\n");
#endif
    putctl1(WR(q)->q_next, M_CTL, MC_SERVICEIMM);
    zsunbufopened = 1;
    return(0);
}

static int
zsunbufwput(q, mp)
    queue_t *q;
    mblk_t *mp;
{
    putnext(q, mp);
}

static int
zsunbufrput(q, mp)
    queue_t *q;
    mblk_t *mp;
{
    putnext(q, mp);
}

static int
zsunbufclose(q, flag)
    queue_t *q;
    int flag;
{
#ifdef DEBUG
    log(LOG_INFO, "zsunbufclose: set MC_SERVICEDEF\n");
#endif
    putctl1(WR(q)->q_next, M_CTL, MC_SERVICEDEF);
    return(0);
}

#ifdef LOADABLE

/* Allow the module to be loaded dynamically.  This is a hack, but
 * much more convenient than building a new kernel.  The module
 * cannot be unloaded, but you can always change the name if
 * you want to load a new version.
 * I stole this code from dp-2.2-beta.
 */

#include <sys/conf.h>
#include <sys/buf.h>
#include <sundev/mbvar.h>
#include <sun/autoconf.h>
#include <sun/vddrv.h>
#include <sys/errno.h>

static struct vdldrv zsunbuf_vd = {
    VDMAGIC_PSEUDO, "zsunbuf streams module", NULL, NULL, NULL, 0, 0,
};

int
zsunbufinit(fc, vdp, vdi, vds)
    unsigned int fc;
    struct vddrv *vdp;
    addr_t vdi;
    struct vdstat *vds;
{
    static struct fmodsw *fmod_zsunbuf;

    switch(fc) {
        case VDLOAD:
            {
                int dev,i;
                for(dev = 0; dev < fmodcnt; dev++) {
                    if(fmodsw[dev].f_str == NULL)
                        break;
                }
                if(dev == fmodcnt)
                    return(ENODEV);
                fmod_zsunbuf = &fmodsw[dev];
                for(i = 0; i <= FMNAMESZ; i++)
                    fmod_zsunbuf->f_name[i] = minfo.mi_idname[i];
                fmod_zsunbuf->f_str = &zsunbufinfo;
                vdp->vdd_vdtab = (struct vdlinkage *) &zsunbuf_vd;
            }
#ifdef DEBUG
            log(LOG_INFO, "zsunbuf loaded");
#endif
            return 0;
        case VDUNLOAD:
            /* once the module has been opened any
               attempt to unload it panics the kernel */
            if (zsunbufopened)
                return EIO;
            fmod_zsunbuf->f_name[0] = '\0';
            fmod_zsunbuf->f_str = NULL;
#ifdef DEBUG
            log(LOG_INFO, "zsunbuf unloaded");
#endif
            return 0;
        case VDSTAT:
            return 0;
        default:
            return EIO;
    }
}

#endif /* LOADABLE */
