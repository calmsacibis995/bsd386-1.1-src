#!/usr/bin/perl
#
# Copyright (c) 1993 Berkeley Software Design, Inc. All rights reserved.
# The Berkeley Software Design Inc. software License Agreement specifies
# the terms and conditions for redistribution.
#
#	BSDI $Id: units.pl,v 1.2 1993/03/13 19:50:17 polk Exp $
#
$datafile = "
# basic units

meter
gram
second
coulomb
candela
dollar
radian
bit

# constants and miscellaneous

pi			3.14159265358979323846264338327950288
c			2.99792458e8 m/sec
g			9.80665 m/sec2
au			1.49599e11 m
mole			6.022045e23
e			1.6020e-19 coul
abcoulomb		10 coul
force			g
slug			lb g sec2/ft
mercury			1.3157895 atm/m
hg			mercury
torr			mm hg
%			1|100
percent			%
cg			centigram

atmosphere		1.01325 bar
atm			atmosphere
psi			lb g/in in
bar			1e6 dyne/cm2
barye			1e-6 bar

chemamu			1.66024e-24 g
physamu			1.65979e-24 g
amu			chemamu
chemdalton		chemamu
physdalton		physamu
dalton			amu

dozen			12
bakersdozen		13
quire			25
ream			500
gross			144
hertz			1/sec
cps			hertz
hz			hertz
khz			kilohz
mhz			megahz
rutherford		1e6 /sec
degree			1|180 pi radian
circle			2 pi radian
turn			2 pi radian
revolution		360 degree
rpm			revolution/minute
grade			1|400 circle
grad			1|400 circle
sign			1|12 circle
arcdeg			1 degree
arcmin			1|60 arcdeg
arcsec			1|60 arcmin
karat			1|24
proof			1|200
mpg			mile/gal
refrton			12000 btu/hour
curie			3.7e10 /sec

stoke			1 cm2/sec

# angular volume

steradian		radian radian
sr			steradian
sphere			4 pi steradian

# Time

ps			picosec
us			microsec
ns			nanosec
us			microsec
ms			millisec
sec			second
s			second
minute			60 sec
min			minute
hour			60 min
hr			hour
day			24 hr
week			7 day
quadrant		5400 minute
fortnight		14 day
year			365.24219 day
yr			year
month			1|12 year
mo			month
decade			10 yr
century			100 year
millenium		1000 year

# Mass

gm			gram
myriagram		10 kg
kg			kilogram
mg			milligram
metricton		1000 kg
gamma			1e-6 g
metriccarat		200 mg
quintal			100 kg

# Avoirdupois

lb			0.45359237 kg
pound			lb
lbf			lb g
cental			100 lb
stone			0.14 cental
ounce			1|16 lb
oz			ounce
avdram			1|16 oz
usdram			1|8 oz
dram			avdram
dr			dram
grain			1|7000 lb
gr			grain
shortton		2000 lb
ton			shortton
longquarter		254.0117272 kg
shortquarter		500 pounds
longton			2240 lb
longhundredweight	112 lb
shorthundredweight	100 lb
longquarter		254.0117272 kg
wey			252 pound
carat			205.3 mg

# Apothecary

scruple			20 grain
pennyweight		24 grain
apdram			60 grain
apounce			480 grain
appound			5760 grain

# Length

m			meter
cm			centimeter
mm			millimeter
km			kilometer
parsec			3.08572e13 km
pc			parsec
nm			nanometer
micron			1e-6 meter
angstrom		1e-8 cm
fermi			1e-13 cm

point			1|72.27 in
pica			0.166044 inch
caliber			0.01 in
barleycorn		1|3 in
inch			2.54 cm
in			inch
mil			0.001 in
palm			3 in
hand			4 in
span			9 in
foot			12 in
feet			foot
ft			foot
cubit			18 inch
pace			30 inch
yard			3 ft
yd			yard
fathom			6 ft
rod			16.5 ft
rd			rod
rope			20 ft
ell			45 in
skein			360 feet
furlong			660 ft
cable			720 ft
nmile			1852 m
nautmile		nmile
bolt			120 feet
mile			5280 ft
mi			mile
league			3 mi
nautleague		3 nmile
lightyear		c yr

engineerschain		100 ft
engineerslink		0.01 engineerschain
gunterchain		66 ft
gunterlink		0.01 gunterchain
ramdenchain		100 ft
ramdenlinke		0.01 ramdenchain
chain			gunterchain
link			gunterlink

# area

acre			43560 ft2
rood			0.25 acre
are			100 m2
centare			0.01 are
hectare			100 are
barn			1e-24 cm2
section			mi2
township		36 mi2

# volume

cc			cm3 
liter			1000 cc
l			liter
ml			milliliter
registerton		100 ft3
cord			128 ft3
boardfoot		144 in3
cordfoot		0.125 cord
last			2909.414 l
perch			24.75 ft3
stere			m3
cfs			ft3/sec

# US Liquid

gallon			231 in3
imperial		1.200949
gal			gallon
quart			1|4 gal
qt			quart
magnum			2 qt
pint			1|2 qt
pt			pint
cup			1|2 pt
gill			1|4 pt
fifth			1|5 gal
firkin			72 pint
barrel			31.5 gal
petrbarrel		42 gal
hogshead		63 gal
tun			252 gallon
hd			hogshead
kilderkin		18 imperial gal
noggin			1 imperial gill

floz			1|4 gill
fldr			1|32 gill
tablespoon		4 fldr
teaspoon		1|3 tablespoon
minim			1|480 floz

# US Dry

dry			268.8025 in3/gallon
peck			2 dry gal
pk			peck
bushel			8 dry gal
bu			bushel

# British

british			277.4193|231
brpeck			2 dry british gal
brbucket		4 dry british gal
brbushel		8 dry british gal
brfirkin		1.125 brbushel
puncheon		84 gal
dryquartern		2.272980 l
liqquartern		0.1420613 l
butt			126 gal
bag			3 brbushels
brbarrel		4.5 brbushels
seam			8 brbushels
drachm			3.551531 ml

# Energy Work

newton			kg m/sec2
nt			newton
pascal			nt/m2
joule			nt m
cal			4.18400 joule
gramcalorie		cal
calorie			cal
btu			1054.35 joule
frigorie		kilocal
kcal			kilocal
kcalorie		kilocal
langley			cal/cm cm
dyne			erg/cm
poundal			13825.50 dyne
pdl			poundal
erg			1e-7 joule
horsepower		550 ft lb g/sec
hp			horsepower
poise			gram/cm sec
reyn			6.89476e-6 centipoise
rhe			1/poise

# Electrical

coul			coulomb
statcoul		3.335635e-10 coul
ampere			coul/sec
abampere		10 amp
statampere		3.335635e-10 amp
amp			ampere
watt			joule/sec
volt			watt/amp
v			volt
abvolt			10 volt
statvolt		299.7930 volt
ohm			volt/amp
abohm			10 ohm
statohm			8.987584e11 ohm
mho			/ohm
abmho			10 mho
siemens			/ohm
farad			coul/volt
abfarad			10 farad
statfarad		1.112646e-12 farad
pf			picofarad
henry			sec2/farad
abhenry			10 henry
stathenry		8.987584e11 henry
mh			millihenry
weber			volt sec
maxwell			1e-8 weber
gauss			maxwell/cm2
electronvolt		e volt
ev			e volt
kev			1e3 e volt
mev			1e6 e volt
bev			1e9 e volt
faraday			9.648456e4 coul
gilbert			0.7957747154 amp
oersted			1 gilbert / cm
oe			oersted

# Light

cd			candela
lumen			cd sr
lux			lumen/m2
footcandle		lumen/ft2
footlambert		0.31830989 cd / ft2
lambert			0.31830989 cd / cm cm
phot			lumen/cm2
stilb			cd/cm2

candle			cd
engcandle		1.04 cd
germancandle		1.05 cd
carcel			9.61 cd
hefnerunit		0.90 cd

candlepower		12.566370 lumen


# Computer stuff

baud			bit/sec
byte			8 bit
kb			1024 byte
mb			1024 kb
word			4 byte
long			4 byte
block			512 byte

# Speed

mph			mile/hr
knot			nmile/hour
brknot			6080 ft/hr
mach			331.45 m/sec


# Money
# Reported 2/22/93 Wall Street Journal; Value on 2/19/93
# Be sure to update man page if you change these...RK

#$			dollar
buck			dollar
cent			0.01 dollar
afghanistan_afghani	1|1162.50 dollar
albania_lek		 1|110.00 dollar
algeria_dinar		  1|22.69 dollar
andorra_peseta		 1|117.08 dollar
andorra_frank		   1|5.5307 dollar
angola_newkwanza	1|6965.00 dollar
antigua_ecaribbeandollar	   1|2.70 dollar
argentina_peso		   1|0.9994 dollar
aruba_florin		   1|1.79 dollar
australia_ausdollar	   1|1.4538 dollar
austria_schilling	  1|11.4875 dollar
bahamas_dollar		   1|1.00 dollar
bahrain_dinar		   1|0.377 dollar
bangladesh_taka		  1|38.95 dollar
barbados_dollar		   1|2.0113 dollar
belgium_franc		  1|33.63 dollar
belize_dollar		   1|2.00 dollar
benin_cfafranc		 1|276.535 dollar
bermuda_usdollar	   1|1.00 dollar
bhutan_ngultrum		  1|30.08 dollar
bolivia_boliviano	   1|4.14 dollar
botswana_pula		   1|2.3076
bouvetisland_norkrone	   1|6.9425 dollar
brazil_cruzeiro	       1|18871.55 dollar
brunei(dollar)		   1|1.6458 dollar
bulgaria_lev		  1|26.502 dollar
burkinafaso_cfafranc	 1|276.535 dollar
burma_kyat		   1|6.2707 dollar
burundi_franc		 1|220.6002 dollar
cambodia_riel		1|2000.00 dollar
cameroon_cfafranc	 1|276.535 dollar
canada_dollar		   1|1.2583 dollar
capeverdeisl_escudo	  1|74.20 dollar
caymanislands_dollar	   1|0.85 dollar
centralafrrep_cfafranc   1|276.535 dollar
chad_cfafranc		 1|276.535 dollar
chile_peso		 1|387.75 dollar
chile_officialpeso	 1|424.06 dollar
china_renminbiyuan	   1|5.0755 dollar
colombia_peso	 	 1|826.16 dollar
cis_rouble		 1|559.00 dollar
comoros_cfafranc	 1|276.535 dollar
congo_cfafranc		 1|276.535 dollar
costarica_colon		 1|138.34 dollar
cuba_peso		   1|1.3203 dollar
cyprus_pound		   1|2.0405 dollar
czech_koruna		  1|28.989 dollar
denmark_dankrone	   1|6.265 dollar
djibouti_franc		 1|177.72 dollar
dominica_ecaribbeandollar	   1|2.70 dollar
dominicanrep_peso	  1|13.00 dollar
ecuador_sucre		1|1850.00 dollar
egypt_pound		   1|3.3405 dollar
elsalvador_colon	   1|8.82 dollar
eqguinea_cfafranc	 1|276.535 dollar
estonia_kroon		  1|13.10 dollar
ethiopia_birr		   1|5.00 dollar
faeroeisl_dankrone   	   1|6.265 dollar
falklandisl_pound	   1|1.4511 dollar
fiji_dollar		   1|1.5738 dollar
finland_markka		   1|5.5825 dollar
france_franc		   1|5.5307 dollar
frguiana_franc		   1|5.5307 dollar
frpacificisl_cfpfranc	 1|100.5581 dollar
gabon_cfafranc		 1|276.535 dollar
gambia_dalasi		   1|8.741 dollar
germany_mark		   1|1.6333 dollar
ghana_cedi		 1|575.00 dollar
gibraltar_pound		   1|1.4511 dollar
greece_drachma		 1|219.60 dollar
greenland_dankrone	   1|6.265 dollar
grenada_ecaribbeandollar	   1|2.70 dollar
guadeloupe_franc	   1|5.5307 dollar
guam_usdollar		   1|1.00 dollar
guatemala_quetzal	   1|5.37 dollar
guineabissau_peso	1|5000.00 dollar
guinearep_franc		 1|812.29 dollar
guyana_dollar		 1|126.00 dollar
haiti_gourde		  1|12.00 dollar
hondurasrep_lempira	   1|5.89 dollar
honkong_dollar		   1|7.7325 dollar
hungary_forint		  1|86.43 dollar
iceland_krona		  1|64.67 dollar
india_rupee		  1|30.08 dollar
indonesia_rupiah	1|2064.00 dollar
iran_rial		1|1495.00 dollar
iraq_dinar		   1|0.3125 dollar
ireland_punt		   1|1.4935 dollar
israel_newshekel	   1|2.809 dollar
italy_lira		1|1563.25 dollar
ivorycoast_cfa_franc	 1|276.535 dollar
jamaica_dollar		  1|22.07 dollar
japan_yen		 1|119.30 dollar
jordan_dinar		   1|0.691 dollar
kenya_shilling		  1|36.0645 dollar
kiribati_ausdollar	   1|1.4538 dollar
northkorea_won		   1|2.15 dollar
southkorea_won		 1|796.80 dollar
kuwait_dinar		   1|0.3604 dollar
laos_kip		 1|720.00 dollar
lebanon_pound		1|1748.00 dollar
lesotho_maloti		   1|3.1193 dollar
liberia_dollar		   1|1.00 dollar
libya_dinar		   1|0.2896 dollar
liechtenstein_franc	   1|1.5045 dollar
luxembourg_luxfranc	  1|33.63 dollar
macao_pataca		   1|7.9877 dollar
madagascar_franc	1|1892.63 dollar
malawi_kwacha		   1|4.4539 dollar
malaysia_ringgit	   1|2.629 dollar
maldive_rufiyaa		  1|11.975 dollar
malirep_cfafranc	 1|276.535 dollar
malta_lira		   1|2.6259 dollar
martinique_franc	   1|5.5307 dollar
mauritania_ouguiya	 1|106.00 dollar
mauritius_rupee		  1|17.20 dollar
mexico_newpeso		   1|3.0935 dollar
monaco_franc		   1|5.5307 dollar
mongolia_tugrik		 1|150.00 dollar
montserrat_ecaribbeandollar	   1|2.70 dollar
morocco_dirham		   1|8.9105 dollar
mozambique_metical	1|2753.59 dollar
namibia_rand		   1|3.1193 dollar
nauruisl_ausdollar	   1|1.4538 dollar
nepal_rupee		  1|46.63 dollar
netherlands_guilder	   1|1.839 dollar
nethantilles_guilder 	   1|1.79 dollar
newzealand_nzdollar	   1|1.9344 dollar
nicaragua_goldcordoba	   1|6.00 dollar
nigerrep_cfa_franc	 1|276.535 dollar
nigeria_naira		  1|24.99 dollar
norway_norkrone		   1|6.9425 dollar
oman_rial		   1|0.385 dollar
pakistan_rupee		  1|26.32 dollar
panama_balboa		   1|1.00 dollar
papuang_kina		   1|0.9842 dollar
paraguay_guarani	1|1658.00 dollar
peru_newsol		   1|1.76 dollar
philippines_peso	  1|25.326 dollar
pitcairnisl_nzdollar	   1|1.9344 dollar
poland_zloty	       1|16463.00 dollar
portugal_escudo		 1|149.45 dollar
puertorico_usdollar	   1|1.00 dollar
qatar_riyal		   1|3.64 dollar
yemen_offrial		  1|18.00 dollar
yemen_dinar		   1|0.465 dollar
yemen_rial		  1|16.50 dollar
reunionisl_franc	   1|5.5307 dollar
romania_leu		 1|530.00 dollar
rwanda_franc		 1|147.1472 dollar
stchristopher_ecaribbeandollar  1|2.70 dollar
sthelena_poundsterling	   1|1.4511 dollar
stlucia_ecaribbeandollar	   1|2.70 dollar
stpierre_franc		   1|5.5307 dollar
stvincent_ecaribbeandollar	   1|2.70 dollar
amersamoa_usdollar	   1|1.00 dollar
westernsamoa_tala	   1|2.5575 dollar
sanmarino_lira		1|1563.25 dollar
saotomeprincipe_dobra	 1|240.00 dollar
saudiarabia__riyal	   1|3.7503 dollar
senegal_cfa_franc	 1|276.535 dollar
seychelles_rupee	   1|5.2789 dollar
sierraleone_leone	 1|535.00 dollar
singapore_dollar	   1|1.6458 dollar
slovakrep_koruna	  1|28.989 dollar
slovenia_tolar		 1|102.38 dollar
solomonisl_soldollar	   1|3.1027 dollar
somalirep_shilling	1|2620.00 dollar
southafrica_finrand	   1|4.588 dollar
southafrica_commrand	   1|3.1193 dollar
spain_peseta		 1|117.08 dollar
srilanka_rupee		  1|46.28 dollar
sudanrep_dinar		  1|10.00 dollar
sudanrep_pound		 1|100.00 dollar
surinam_guilder		   1|1.785 dollar
swaziland_lilangeni	   1|3.1193 dollar
sweden_krona		   1|7.575 dollar
switzerland_franc	   1|1.5045 dollar
syria_pound		  1|21.00 dollar
taiwan_dollar		  1|25.94 dollar
tanzania_shilling	 1|350.26 dollar
thailand_baht		  1|25.51 dollar
togo_cfafranc		 1|276.535 dollar
tongaisl_paanga		   1|1.4538 dollar
trinidadtobago_dollar	   1|4.25 dollar
tunisia_dinar		   1|0.9946 dollar
turkey_lira		1|9228.94 dollar
turkscaicos_usdollar	   1|1.00 dollar
tuvalu_ausdollar	   1|1.4538 dollar
uganda_shilling		1|1217.36 dollar
uar_dirham		   1|3.761 dollar
uk_poundsterling	   1|1.4511 dollar
uruguay_peso		1|3601.00 dollar
vanuatu_vatu		 1|119.25 dollar
vaticancity_lira	1|1563.25 dollar
venezuela_bolivar	  1|82.35 dollar
vietnam_dong	       1|10510.00 dollar
britvirgisl_usdollar	   1|1.00 dollar
usvirgisl_usdollar	   1|1.00 dollar
yugoslavia_newdinar	 1|750.00 dollar
zaire_zaire	     1|2652000.00 dollar
zambia_kwacha		 1|435.24 dollar
zimbabwe_dollar		   1|6.4021 dollar

";

# read the conversions into the %convert:

@lines = split (/\n/, $datafile);
foreach (@lines) {
	(/^#/ || /^$/) && next;
	($left, $right) = split (/[ 	]+/, $_, 2);
	if ($right eq "") {
		$basicunit{$left} = 1;
		next;
	}
	$convert{$left} = $right;
}

for (;;) {
	$value = 1.0;
	%unittop = ();

	print "you have: ";
	chop ($have = <>);
	if($have eq "") { print "\n"; exit; }
	next if &dounit ($have, 1);

	for(;;) {
	    print "you want: ";
	    chop ($want = <>);
	    if($want eq "") { print "\n"; exit; }
	    last if &dounit ($want, 0)==0;
	}

# check to make sure that the numerator and denominator of
# both the `have' and `want' have the same dimensions:

	$bad = 0;
	foreach (keys %basicunit) {
		$bad = 1 if $unittop{$_};
	}
	if ($bad) { print "Non-conforming values\n"; }
	else  {
		printf ("        * %.6e\n", $value);
		printf ("        / %.6e\n", 1.0/$value);
	}
}

sub dounit {
	local ($in, $top, $try, $newunit, @pieces) = @_;

# break line into managable set of number and units -- @pieces

        $in =~ s,/, / ,g;
	$in =~ s/(\d[\d\.eE\+\-\|]*)/\1 /;
# maybe here we should: s/-/ /g;
	@pieces = split (/[ 	]+/, $in);
	while ($_ = shift @pieces) {
		if ($_ eq "/") { $top = 1 - $top; next; } # time to invert...
		if ($_ =~ /^[\d\.]/) {	# the input number
			if (/\|/) {	# a fractional input number
				s,\|,/,;
				$_ = eval("$_");
			}
			$value = $top ? $value * $_ : $value / $_;
			next;
		}
		if (/\d$/) {		# cm3 -> cm cm cm
			/^(\D+)(\d+)$/;
			($_, $power) =  ($1, $2);
			while (--$power) { unshift (@pieces, $_); }
		}
# prefixes (updated with 1993 additions!)
# While this could be an array and a loop, this scheme should be faster.

   for(;;) {
	if (s/^yocto//) { $value = $top? $value/1e24: $value * 1e24; next; }
	if (s/^zepto//) { $value = $top? $value/1e21: $value * 1e21; next; }
	if (s/^atto//)  { $value = $top? $value/1e18: $value * 1e18; next; }
	if (s/^femto//) { $value = $top? $value/1e12: $value * 1e15; next; }
	if (s/^pico//)  { $value = $top? $value/1e12: $value * 1e12; next; }
	if (s/^nano//)  { $value = $top? $value/1e9 : $value * 1e9;  next; }
	if (/^microns*$/) { last; }
	if (s/^micro//) { $value = $top? $value/1e6 : $value * 1e6;  next; }
	if (s/^milli//) { $value = $top? $value/1e3 : $value * 1e3;  next; }
	if (s/^centi//) { $value = $top? $value/1e2 : $value * 1e2;  next; }
	if (s/^deci//)  { $value = $top? $value/1e1 : $value * 1e1;  next; }
	if (s/^deca//)  { $value = $top? $value*1e1 : $value / 1e1;  next; }
	if (s/^hecto//) { $value = $top? $value*1e2 : $value / 1e2;  next; }
	if (s/^kilo//)  { $value = $top? $value*1e3 : $value / 1e3;  next; }
	if (s/^mega//)  { $value = $top? $value*1e6 : $value / 1e6;  next; }
	if (s/^giga//)  { $value = $top? $value*1e9 : $value / 1e9;  next; }
	if (s/^tera//)  { $value = $top? $value*1e12: $value / 1e12; next; }
	if (s/^peta//)  { $value = $top? $value*1e15: $value / 1e15; next; }
	if (s/^exa//)   { $value = $top? $value*1e18: $value / 1e18; next; }
	if (s/^zetta//) { $value = $top? $value*1e21: $value / 1e21; next; }
	if (s/^yotta//) { $value = $top? $value*1e24: $value / 1e24; next; }
	last;
   }

		if ($basicunit{$_}) {	# basic unit -- track its dimensions
			$unittop{$_} += $top ? +1 : -1;
			next;
		}

		if ($convert{$_}) { 	# convertible unit
			&dounit ($convert{$_}, $top);
			next;
		}
		# convert from plural; unfortunate that we can't
		# leverage above 2 blocks
		$try = $_;
		$try =~ s/s$//;
					# below is same as previous 2 blocks
		if ($basicunit{$try}) {
			$unittop{$try} += $top ? +1 : -1;
			next;
		}
		if ($convert{$try}) {
			&dounit ($convert{$try}, $top);
			next;
		}
		print "Don't recognize `$_'\n";
		return 1;
	}
	return 0;
}
