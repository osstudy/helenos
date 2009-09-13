/*
 * Copyright (c) 2009 Jiri Svoboda
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * - Redistributions of source code must retain the above copyright
 *   notice, this list of conditions and the following disclaimer.
 * - Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimer in the
 *   documentation and/or other materials provided with the distribution.
 * - The name of the author may not be used to endorse or promote products
 *   derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/** @addtogroup edit
 * @brief Text editor.
 * @{
 */
/**
 * @file
 */

#include <stdio.h>
#include <sys/types.h>
#include <vfs/vfs.h>
#include <io/console.h>
#include <io/color.h>
#include <io/keycode.h>
#include <errno.h>
#include <align.h>
#include <macros.h>
#include <bool.h>

#include "sheet.h"

enum redraw_flags {
	REDRAW_TEXT	= (1 << 0),
	REDRAW_ROW	= (1 << 1),
	REDRAW_STATUS	= (1 << 2),
	REDRAW_CARET	= (1 << 3)
};

/** Pane
 *
 * A rectangular area of the screen used to edit a document. Different
 * panes can be possibly used to edit the same document.
 */
typedef struct {
	/* Pane dimensions */
	int rows, columns;

	/* Position of the visible area */
	int sh_row, sh_column;

	/** Bitmask of components that need redrawing */
	enum redraw_flags rflags;

	/** Current position of the caret */
	tag_t caret_pos;
} pane_t;

/** Document
 *
 * Associates a sheet with a file where it can be saved to.
 */
typedef struct {
	char *file_name;
	sheet_t sh;
} doc_t;

static int con;
static doc_t doc;
static bool done;
static pane_t pane;

static ipcarg_t scr_rows, scr_columns;

#define ROW_BUF_SIZE 4096
#define BUF_SIZE 64
#define TAB_WIDTH 8
#define ED_INFTY 65536

static void key_handle_unmod(console_event_t const *ev);
static void key_handle_ctrl(console_event_t const *ev);
static int file_save(char const *fname);
static int file_insert(char *fname);
static int file_save_range(char const *fname, spt_t const *spos,
    spt_t const *epos);
static void pane_text_display(void);
static void pane_row_display(void);
static void pane_row_range_display(int r0, int r1);
static void pane_status_display(void);
static void pane_caret_display(void);
static void insert_char(wchar_t c);
static void delete_char_before(void);
static void delete_char_after(void);
static void caret_update(void);
static void caret_move(int drow, int dcolumn, enum dir_spec align_dir);
static void pt_get_sof(spt_t *pt);
static void pt_get_eof(spt_t *pt);
static void status_display(char const *str);


int main(int argc, char *argv[])
{
	console_event_t ev;
	coord_t coord;
	bool new_file;

	spt_t pt;

	con = fphone(stdout);
	console_clear(con);

	console_get_size(con, &scr_columns, &scr_rows);

	pane.rows = scr_rows - 1;
	pane.sh_row = 1;

	/* Start with an empty sheet. */
	sheet_init(&doc.sh);

	/* Place caret at the beginning of file. */
	coord.row = coord.column = 1;
	sheet_get_cell_pt(&doc.sh, &coord, dir_before, &pt);
	sheet_place_tag(&doc.sh, &pt, &pane.caret_pos);

	if (argc == 2) {
		doc.file_name = argv[1];
	} else if (argc > 1) {
		printf("Invalid arguments.\n");
		return -2;
	} else {
		doc.file_name = "/edit.txt";
	}

	new_file = false;

	if (file_insert(doc.file_name) != EOK)
		new_file = true;

	/* Move to beginning of file. */
	caret_move(-ED_INFTY, -ED_INFTY, dir_before);

	/* Initial display */
	console_clear(con);
	pane_text_display();
	pane_status_display();
	if (new_file)
		status_display("File not found. Created empty file.");
	pane_caret_display();


	done = false;

	while (!done) {
		console_get_event(con, &ev);
		pane.rflags = 0;

		if (ev.type == KEY_PRESS) {
			/* Handle key press. */
			if (((ev.mods & KM_ALT) == 0) &&
			     (ev.mods & KM_CTRL) != 0) {
				key_handle_ctrl(&ev);
			} else if ((ev.mods & (KM_CTRL | KM_ALT)) == 0) {
				key_handle_unmod(&ev);
			}
		}

		/* Redraw as necessary. */

		if (pane.rflags & REDRAW_TEXT)
			pane_text_display();
		if (pane.rflags & REDRAW_ROW)
			pane_row_display();
		if (pane.rflags & REDRAW_STATUS)
			pane_status_display();
		if (pane.rflags & REDRAW_CARET)
			pane_caret_display();
			
	}

	console_clear(con);

	return 0;
}

/** Handle key without modifier. */
static void key_handle_unmod(console_event_t const *ev)
{
	switch (ev->key) {
	case KC_ENTER:
		insert_char('\n');
		pane.rflags |= REDRAW_TEXT;
		caret_update();
		break;
	case KC_LEFT:
		caret_move(0, -1, dir_before);
		break;
	case KC_RIGHT:
		caret_move(0, 0, dir_after);
		break;
	case KC_UP:
		caret_move(-1, 0, dir_before);
		break;
	case KC_DOWN:
		caret_move(+1, 0, dir_before);
		break;
	case KC_HOME:
		caret_move(0, -ED_INFTY, dir_before);
		break;
	case KC_END:
		caret_move(0, +ED_INFTY, dir_before);
		break;
	case KC_PAGE_UP:
		caret_move(-pane.rows, 0, dir_before);
		break;
	case KC_PAGE_DOWN:
		caret_move(+pane.rows, 0, dir_before);
		break;
	case KC_BACKSPACE:
		delete_char_before();
		pane.rflags |= REDRAW_TEXT;
		caret_update();
		break;
	case KC_DELETE:
		delete_char_after();
		pane.rflags |= REDRAW_TEXT;
		caret_update();
		break;
	default:
		if (ev->c >= 32 || ev->c == '\t') {
			insert_char(ev->c);
			pane.rflags |= REDRAW_ROW;
			caret_update();
		}
		break;
	}
}

/** Handle Ctrl-key combination. */
static void key_handle_ctrl(console_event_t const *ev)
{
	switch (ev->key) {
	case KC_Q:
		done = true;
		break;
	case KC_S:
		(void) file_save(doc.file_name);
		break;
	default:
		break;
	}
}


/** Save the document. */
static int file_save(char const *fname)
{
	spt_t sp, ep;
	int rc;

	status_display("Saving...");
	pt_get_sof(&sp);
	pt_get_eof(&ep);

	rc = file_save_range(fname, &sp, &ep);
	status_display("File saved.");

	return rc;
}

/** Insert file at caret position.
 *
 * Reads in the contents of a file and inserts them at the current position
 * of the caret.
 */
static int file_insert(char *fname)
{
	FILE *f;
	wchar_t c;
	char buf[BUF_SIZE];
	int bcnt;
	int n_read;
	size_t off;

	f = fopen(fname, "rt");
	if (f == NULL)
		return EINVAL;

	bcnt = 0;

	while (true) {
		if (bcnt < STR_BOUNDS(1)) {
			n_read = fread(buf + bcnt, 1, BUF_SIZE - bcnt, f);
			bcnt += n_read;
		}

		off = 0;
		c = str_decode(buf, &off, bcnt);
		if (c == '\0')
			break;

		bcnt -= off;
		memcpy(buf, buf + off, bcnt);

		insert_char(c);
	}

	fclose(f);

	return EOK;
}

/** Save a range of text into a file. */
static int file_save_range(char const *fname, spt_t const *spos,
    spt_t const *epos)
{
	FILE *f;
	char buf[BUF_SIZE];
	spt_t sp, bep;
	size_t bytes, n_written;

	f = fopen(fname, "wt");
	if (f == NULL)
		return EINVAL;

	sp = *spos;

	do {
		sheet_copy_out(&doc.sh, &sp, epos, buf, BUF_SIZE, &bep);
		bytes = str_size(buf);

		n_written = fwrite(buf, 1, bytes, f);
		if (n_written != bytes) {
			return EIO;
		}

		sp = bep;
	} while (!spt_equal(&bep, epos));

	fclose(f);

	return EOK;
}

static void pane_text_display(void)
{
	int sh_rows, rows;
	int i;
	unsigned j;

	sheet_get_num_rows(&doc.sh, &sh_rows);
	rows = min(sh_rows - pane.sh_row + 1, pane.rows);

	/* Draw rows from the sheet. */

	console_goto(con, 0, 0);
	pane_row_range_display(0, rows);

	/* Clear the remaining rows if file is short. */

	for (i = rows; i < pane.rows; ++i) {
		console_goto(con, 0, i);
		for (j = 0; j < scr_columns; ++j)
			putchar(' ');
		fflush(stdout);
	}

	pane.rflags |= (REDRAW_STATUS | REDRAW_CARET);
	pane.rflags &= ~REDRAW_ROW;
}

/** Display just the row where the caret is. */
static void pane_row_display(void)
{
	spt_t caret_pt;
	coord_t coord;
	int ridx;

	tag_get_pt(&pane.caret_pos, &caret_pt);
	spt_get_coord(&caret_pt, &coord);

	ridx = coord.row - pane.sh_row;
	pane_row_range_display(ridx, ridx + 1);
	pane.rflags |= (REDRAW_STATUS | REDRAW_CARET);
}

static void pane_row_range_display(int r0, int r1)
{
	int width;
	int i, j, fill;
	spt_t rb, re, dep;
	coord_t rbc, rec;
	char row_buf[ROW_BUF_SIZE];
	wchar_t c;
	size_t pos, size;
	unsigned s_column;

	/* Draw rows from the sheet. */

	console_goto(con, 0, 0);
	for (i = r0; i < r1; ++i) {
		sheet_get_row_width(&doc.sh, pane.sh_row + i, &width);

		/* Determine row starting point. */
		rbc.row = pane.sh_row + i; rbc.column = 1;
		sheet_get_cell_pt(&doc.sh, &rbc, dir_before, &rb);

		/* Determine row ending point. */
		rec.row = pane.sh_row + i; rec.column = width + 1;
		sheet_get_cell_pt(&doc.sh, &rec, dir_before, &re);

		/* Copy the text of the row to the buffer. */
		sheet_copy_out(&doc.sh, &rb, &re, row_buf, ROW_BUF_SIZE, &dep);

		/* Display text from the buffer. */

		console_goto(con, 0, i);
		size = str_size(row_buf);
		pos = 0;
		s_column = 1;
		while (pos < size) {
			c = str_decode(row_buf, &pos, size);
			if (c != '\t') {
				printf("%lc", c);
				s_column += 1;
			} else {
				fill = 1 + ALIGN_UP(s_column, TAB_WIDTH)
				    - s_column;

				for (j = 0; j < fill; ++j)
					putchar(' ');
				s_column += fill;
			}
		}

		/* Fill until the end of display area. */

		if (str_length(row_buf) < scr_columns)
			fill = scr_columns - str_length(row_buf);
		else
			fill = 0;

		for (j = 0; j < fill; ++j)
			putchar(' ');
		fflush(stdout);
	}

	pane.rflags |= REDRAW_CARET;
}

/** Display pane status in the status line. */
static void pane_status_display(void)
{
	spt_t caret_pt;
	coord_t coord;
	int n;

	tag_get_pt(&pane.caret_pos, &caret_pt);
	spt_get_coord(&caret_pt, &coord);

	console_goto(con, 0, scr_rows - 1);
	console_set_color(con, COLOR_WHITE, COLOR_BLACK, 0);
	n = printf(" %d, %d: File '%s'. Ctrl-S Save  Ctrl-Q Quit",
	    coord.row, coord.column, doc.file_name);
	printf("%*s", scr_columns - 1 - n, "");
	fflush(stdout);
	console_set_color(con, COLOR_BLACK, COLOR_WHITE, 0);

	pane.rflags |= REDRAW_CARET;
}

/** Set cursor to reflect position of the caret. */
static void pane_caret_display(void)
{
	spt_t caret_pt;
	coord_t coord;

	tag_get_pt(&pane.caret_pos, &caret_pt);

	spt_get_coord(&caret_pt, &coord);
	console_goto(con, coord.column - 1, coord.row - pane.sh_row);
}

/** Insert a character at caret position. */
static void insert_char(wchar_t c)
{
	spt_t pt;
	char cbuf[STR_BOUNDS(1) + 1];
	size_t offs;

	tag_get_pt(&pane.caret_pos, &pt);

	offs = 0;
	chr_encode(c, cbuf, &offs, STR_BOUNDS(1) + 1);
	cbuf[offs] = '\0';

	(void) sheet_insert(&doc.sh, &pt, dir_before, cbuf);
}

/** Delete the character before the caret. */
static void delete_char_before(void)
{
	spt_t sp, ep;
	coord_t coord;

	tag_get_pt(&pane.caret_pos, &ep);
	spt_get_coord(&ep, &coord);

	coord.column -= 1;
	sheet_get_cell_pt(&doc.sh, &coord, dir_before, &sp);

	(void) sheet_delete(&doc.sh, &sp, &ep);
}

/** Delete the character after the caret. */
static void delete_char_after(void)
{
	spt_t sp, ep;
	coord_t coord;

	tag_get_pt(&pane.caret_pos, &sp);
	spt_get_coord(&sp, &coord);

	sheet_get_cell_pt(&doc.sh, &coord, dir_after, &ep);

	(void) sheet_delete(&doc.sh, &sp, &ep);
}

/** Scroll pane after caret has moved.
 *
 * After modifying the position of the caret, this is called to scroll
 * the pane to ensure that the caret is in the visible area.
 */
static void caret_update(void)
{
	spt_t pt;
	coord_t coord;

	tag_get_pt(&pane.caret_pos, &pt);
	spt_get_coord(&pt, &coord);

	/* Scroll pane as necessary. */

	if (coord.row < pane.sh_row) {
		pane.sh_row = coord.row;
		pane.rflags |= REDRAW_TEXT;
	}
	if (coord.row > pane.sh_row + pane.rows - 1) {
		pane.sh_row = coord.row - pane.rows + 1;
		pane.rflags |= REDRAW_TEXT;
	}

	pane.rflags |= (REDRAW_CARET | REDRAW_STATUS);

}

/** Change the caret position.
 *
 * Moves caret relatively to the current position. Looking at the first
 * character cell after the caret and moving by @a drow and @a dcolumn, we get
 * to a new character cell, and thus a new character. Then we either go to the
 * point before the the character or after it, depending on @a align_dir.
 */
static void caret_move(int drow, int dcolumn, enum dir_spec align_dir)
{
	spt_t pt;
	coord_t coord;
	int num_rows;

	tag_get_pt(&pane.caret_pos, &pt);
	spt_get_coord(&pt, &coord);
	coord.row += drow; coord.column += dcolumn;

	/* Clamp coordinates. */
	if (drow < 0 && coord.row < 1) coord.row = 1;
	if (dcolumn < 0 && coord.column < 1) coord.column = 1;
	if (drow > 0) {
		sheet_get_num_rows(&doc.sh, &num_rows);
		if (coord.row > num_rows) coord.row = num_rows;
	}

	/*
	 * Select the point before or after the character at the designated
	 * coordinates. The character can be wider than one cell (e.g. tab).
	 */
	sheet_get_cell_pt(&doc.sh, &coord, align_dir, &pt);
	sheet_remove_tag(&doc.sh, &pane.caret_pos);
	sheet_place_tag(&doc.sh, &pt, &pane.caret_pos);

	caret_update();
}


/** Get start-of-file s-point. */
static void pt_get_sof(spt_t *pt)
{
	coord_t coord;

	coord.row = coord.column = 1;
	sheet_get_cell_pt(&doc.sh, &coord, dir_before, pt);
}

/** Get end-of-file s-point. */
static void pt_get_eof(spt_t *pt)
{
	coord_t coord;
	int num_rows;

	sheet_get_num_rows(&doc.sh, &num_rows);
	coord.row = num_rows;
	coord.column = 1;

	sheet_get_cell_pt(&doc.sh, &coord, dir_after, pt);
}

/** Display text in the status line. */
static void status_display(char const *str)
{
	console_goto(con, 0, scr_rows - 1);
	console_set_color(con, COLOR_WHITE, COLOR_BLACK, 0);
	printf(" %*s ", -(scr_columns - 3), str);
	fflush(stdout);
	console_set_color(con, COLOR_BLACK, COLOR_WHITE, 0);

	pane.rflags |= REDRAW_CARET;
}

/** @}
 */
