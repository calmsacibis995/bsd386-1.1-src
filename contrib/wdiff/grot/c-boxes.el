;;; Boxed comments for C mode.
;;; Copyright (C) 1991 Free Software Foundation, Inc.
;;; Francois Pinard <pinard@iro.umontreal.ca>, April 1991.
;;;
;;; I use this hack by putting, in my .emacs file:
;;;
;;;	(setq c-mode-hook
;;;	      '(lambda ()
;;;		 (define-key c-mode-map "\M-q" 'reindent-c-comment)))
;;;	(autoload 'reindent-c-comment "c-boxes" nil t)
;;;
;;; The cursor should be within a comment before reindent-c-comment to
;;; be given, or else it should be between two comments, in which case
;;; the command applies to the next comment.  When the command is
;;; given without prefix, the current comment box type is recognized
;;; and preserved.  Given 0 as a prefix, the comment box disappears
;;; and the comment stays between a single opening `/*' and a single
;;; closing `*/'.  Given 1 or 2 as a prefix, a single or doubled lined
;;; comment box is forced.  Given 3 as a prefix, a Taarna style box is
;;; forced, but you do not even want to hear about those.
;;;
;;; I observed rounded corners first in some code from Warren Tucker
;;; <wht@n4hgf.Mt-Park.GA.US>.

(defvar c-mode-taarna-style nil "*Non-nil for Taarna team C-style.")
(defvar c-comment-box-style 'single "*Preferred style for box comments.")

;;; Set or reset the Taarna team C mode style.

(defun taarna-mode ()
  (interactive)
  (if c-mode-taarna-style
      (progn

	(setq c-mode-taarna-style nil)
	(setq c-indent-level 2)
	(setq c-continued-statement-offset 2)
	(setq c-brace-offset 0)
	(setq c-argdecl-indent 5)
	(setq c-label-offset -2)
	(setq c-tab-always-indent t)
	(setq c-auto-newline nil)
	(setq c-comment-box-style 'single)
	(message "C mode: GNU style"))
    
    (setq c-mode-taarna-style t)
    (setq c-indent-level 4)
    (setq c-continued-statement-offset 4)
    (setq c-brace-offset -4)
    (setq c-argdecl-indent 4)
    (setq c-label-offset -4)
    (setq c-tab-always-indent t)
    (setq c-auto-newline t)
    (setq c-comment-box-style 'taarna)
    (message "C mode: Taarna style")))

;;; Return the minimum value of the left margin of all lines, or -1 if
;;; all lines are empty.

(defun buffer-left-margin ()
  (let ((margin -1))
    (goto-char (point-min))
    (while (not (eobp))
      (skip-chars-forward " \t")
      (if (not (looking-at "\n"))
	  (setq margin
		(if (< margin 0)
		    (current-column)
		  (min margin (current-column)))))
      (forward-line 1))
    margin))

;;; Return the maximum value of the right margin of all lines.  Any
;;; sentence ending a line has a space guaranteed before the margin.

(defun buffer-right-margin ()
  (let ((margin 0) period)
    (goto-char (point-min))
    (while (not (eobp))
      (end-of-line)
      (if (bobp)
	  (setq period 0)
	(backward-char 1)
	(setq period (if (looking-at "[.?!]") 1 0))
	(forward-char 1))
      (setq margin (max margin (+ (current-column) period)))
      (forward-char 1))
    margin))

;;; Indent or reindent a C comment box.  

(defun reindent-c-comment (flag)
  (interactive "P")
  (save-restriction
    (let ((marked-point (point-marker))
	  (saved-point (point))
	  box-style left-margin right-margin)

      ;; First, find the limits of the block of comments following or
      ;; enclosing the cursor, or return an error if the cursor is not
      ;; within such a block of comments, narrow the buffer, and
      ;; untabify it.

      ;; - insure the point is into the following comment, if any

      (skip-chars-forward " \t\n")
      (if (looking-at "/\\*")
	  (forward-char 2))

      (let ((here (point)) start end temp)

	;; - identify a minimal comment block

	(search-backward "/*")
	(setq temp (point))
	(beginning-of-line)
	(setq start (point))
	(skip-chars-forward " \t")
	(if (< (point) temp)
	    (progn
	      (goto-char saved-point)
	      (error "text before comment's start")))
	(search-forward "*/")
	(setq temp (point))
	(end-of-line)
	(forward-char 1)
	(setq end (point))
	(skip-chars-backward " \t\n")
	(if (> (point) temp)
	    (progn
	      (goto-char saved-point)
	      (error "text after comment's end")))
	(if (< end here)
	    (progn
	      (goto-char saved-point)
	      (error "outside any comment block")))

	;; - try to extend the comment block backwards

	(goto-char start)
	(while (and (not (bobp))
		    (progn (previous-line 1)
			   (beginning-of-line)
			   (looking-at "[ \t]*/\\*.*\\*/[ \t]*$")))
	  (setq start (point)))

	;; - try to extend the comment block forward

	(goto-char end)
	(while (looking-at "[ \t]*/\\*.*\\*/[ \t]*$")
	  (forward-line 1)
	  (beginning-of-line)
	  (setq end (point)))

	;; - narrow the whole block of comments

	(narrow-to-region start end))

      ;; Second, remove all the comment marks, and move all the text
      ;; rigidly to the left to insure the left margin stays at the
      ;; same place.  At the same time, recognize and save the box
      ;; style in BOX-STYLE.

      (let ((previous-margin (buffer-left-margin))
	    actual-margin)
    
	;; - remove all comment marks

	(goto-char (point-min))
	(replace-regexp "\\*/[ \t]*/\\*" " ")
	(goto-char (point-min))
	(while (not (eobp))
	  (skip-chars-forward " \t")
	  (if (looking-at "/\\*")
	      (replace-match "  ")
	    (if (looking-at "|")
		(replace-match " ")))
	  (end-of-line)
	  (skip-chars-backward " \t")
	  (backward-char 2)
	  (if (looking-at "\\*/")
	      (replace-match "")
	    (forward-char 1)
	    (if (looking-at "|")
		(replace-match "")
	      (forward-char 1)))
	  (forward-line 1))

	;; - remove the first and last dashed lines

	(setq box-style 'plain)
	(goto-char (point-min))
	(if (looking-at "^[ \t]*-*[.\+\\]?[ \t]*\n")
	    (progn
	      (setq box-style 'single)
	      (replace-match ""))
	  (if (looking-at "^[ \t]*=*[.\+\\]?[ \t]*\n")
	      (progn
		(setq box-style 'double)
		(replace-match ""))))
	(goto-char (point-max))
	(previous-line 1)
	(beginning-of-line)
	(if (looking-at "^[ \t]*[`\+\\]?*[-=]+[ \t]*\n")
	    (progn
	      (if (eq box-style 'plain)
		  (setq box-style 'taarna))
	      (replace-match "")))

	;; - remove all spurious whitespace

	(goto-char (point-min))
	(replace-regexp "[ \t]+$" "")
	(goto-char (point-min))
	(if (looking-at "\n+")
	    (replace-match ""))
	(goto-char (point-max))
	(skip-chars-backward "\n")
	(if (looking-at "\n\n+")
	    (replace-match "\n"))
	(goto-char (point-min))
	(replace-regexp "\n\n\n+" "\n\n")
	
	;; - move the text left is adequate

	(setq actual-margin (buffer-left-margin))
	(if (not (= previous-margin actual-margin))
	    (indent-rigidly (point-min) (point-max)
			    (- previous-margin actual-margin))))

      ;; Third, select the new box style from the old box style and
      ;; the argument, choose the margins for this style and refill
      ;; each paragraph.

      ;; - modify box-style only if flag is defined

      (if flag
	  (setq box-style
		(cond ((eq flag '0) 'plain)
		      ((eq flag '1) 'single)
		      ((eq flag '2) 'double)
		      ((eq flag '3) 'taarna)
		      (c-mode-taarna-style 'taarna)
		      (t 'single))))

      ;; - compute the left margin

      (setq left-margin (buffer-left-margin))

      ;; - temporarily set the fill prefix and column, then refill

      (untabify (point-min) (point-max))
      (let ((fill-prefix (make-string left-margin ? ))
	    (fill-column (- fill-column
			    (if (memq box-style '(single double)) 4 6))))
	(fill-region (point-min) (point-max)))

      ;; - compute the right margin after refill

      (setq right-margin (buffer-right-margin))

      ;; Fourth, put the narrowed buffer back into a comment box,
      ;; according to the value of box-style.  Values may be:
      ;;    plain: insert between a single pair of comment delimiters
      ;;    single: complete box, overline and underline with dashes
      ;;    double: complete box, overline and underline with equal signs
      ;;    taarna: comment delimiters on each line, underline with dashes
	      
      ;; - move the right margin to account for left inserts

      (setq right-margin (+ right-margin
			    (if (memq box-style '(single double))
				2
			      3)))

      ;; - construct the box comment, from top to bottom

      (goto-char (point-min))
      (cond ((eq box-style 'plain)

	     ;; - construct a plain style comment

	     (skip-chars-forward " " (+ (point) left-margin))
	     (insert (make-string (- left-margin (current-column)) ? )
		     "/* ")
	     (end-of-line)
	     (forward-char 1)
	     (while (not (eobp))
	       (skip-chars-forward " " (+ (point) left-margin))
	       (insert (make-string (- left-margin (current-column)) ? )
		       "   ")
	       (end-of-line)
	       (forward-char 1))
	     (backward-char 1)
	     (insert "  */"))
	    ((eq box-style 'single)

	     ;; - construct a single line style comment

	     (indent-to left-margin)
	     (insert "/*")
	     (insert (make-string (- right-margin (current-column)) ?-)
		     "-.\n")
	     (while (not (eobp))
	       (skip-chars-forward " " (+ (point) left-margin))
	       (insert (make-string (- left-margin (current-column)) ? )
		       "| ")
	       (end-of-line)
	       (indent-to right-margin)
	       (insert " |")
	       (forward-char 1))
	     (indent-to left-margin)
	     (insert "`")
	     (insert (make-string (- right-margin (current-column)) ?-)
		     "*/\n"))
	    ((eq box-style 'double)

	     ;; - construct a double line style comment

	     (indent-to left-margin)
	     (insert "/*")
	     (insert (make-string (- right-margin (current-column)) ?=)
		     "=\\\n")
	     (while (not (eobp))
	       (skip-chars-forward " " (+ (point) left-margin))
	       (insert (make-string (- left-margin (current-column)) ? )
		       "| ")
	       (end-of-line)
	       (indent-to right-margin)
	       (insert " |")
	       (forward-char 1))
	     (indent-to left-margin)
	     (insert "\\")
	     (insert (make-string (- right-margin (current-column)) ?=)
		     "*/\n"))
	    ((eq box-style 'taarna)

	     ;; - construct a Taarna style comment

	     (while (not (eobp))
	       (skip-chars-forward " " (+ (point) left-margin))
	       (insert (make-string (- left-margin (current-column)) ? )
		       "/* ")
	       (end-of-line)
	       (indent-to right-margin)
	       (insert " */")
	       (forward-char 1))
	     (indent-to left-margin)
	     (insert "/* ")
	     (insert (make-string (- right-margin (current-column)) ?-)
		     " */\n"))
	    (t (error "unknown box style")))

      ;; Fifth, retabify and restore the point position.

      ; Retabify before left margin only.  Adapted from tabify.el.
      (goto-char (point-min))
      (while (re-search-forward "^[ \t][ \t][ \t]*" nil t)
	(let ((column (current-column))
	      (indent-tabs-mode t))
	  (delete-region (match-beginning 0) (point))
	  (indent-to column)))
      (goto-char (marker-position marked-point)))))
