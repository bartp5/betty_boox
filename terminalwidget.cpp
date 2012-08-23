
/*
 * terminalwidget.cpp
 *
 * Implementation of a libbetty base terminal widget
 *
 * 17.06.2010, Klaus Hennemann
 */

#include "terminalwidget.h"
#include <QColor>
#include <QPainter>
#include <QApplication>

#include "pty.h"
#include <stdio.h>
#include <unistd.h>

/*--------------------------------------------------------------------------*/
/*      Default color palettes                                              */
/*--------------------------------------------------------------------------*/

static const QColor palette_default[] = {
	Qt::black,
	Qt::white
};

static const QColor palette_default_light[] = {
	Qt::white,
	Qt::darkGray
};

static const QColor palette_xterm[] = {
/*	  AARRGGBB */
	0xff000000,	/* black */
	0xffcd0000,	/* red3 */
	0xff00cd00,	/* green3 */
	0xffcdcd00,	/* yellow3 */
	0xff0000ee,	/* blue2 */
	0xffcd00cd,	/* magenta3 */
	0xff00cdcd,	/* cyan3 */
	0xffe5e5e5,	/* gray90 */
	0xff7f7f7f,	/* gray50 */
	0xffff0000,	/* red */
	0xff00ff00,	/* green */
	0xffffff00,	/* yellow */
	0xff5c5cff,
	0xffff00ff,	/* magenta */
	0xff00ffff,	/* cyan */
	0xffffffff	/* white */
};

/*--------------------------------------------------------------------------*/
/*      Widget interface methods                                            */
/*--------------------------------------------------------------------------*/

TerminalWidget::TerminalWidget (QWidget *parent, Qt::WindowFlags f)
	: QWidget(parent, f)
{
	/* we paint all pixels of this widget in our own paint event
	   methods, so we can set WA_OpaquePaintEvent. */
	
	// int id=QFontDatabase::addApplicationFont(":/font/DejaVuSansMono.ttf");
	
	setAttribute(Qt::WA_OpaquePaintEvent);
	setAttribute(Qt::WA_StaticContents);
	setFocusPolicy(Qt::ClickFocus);

	screen_delta.x = 0;
	screen_delta.y = 0;
	cursor_style = BlockCursor;
	cursor_visible = true;

	QFont font("DejaVu Sans Mono");
	font.setPixelSize(14);
	setCellFont(font);

	term = Betty_CreateTerminal(80, 24, 1000);
	Betty_SetUi(term, this, &ui_ops);

	pty = 0;
}


TerminalWidget::~TerminalWidget ()
{
	Betty_DestroyTerminal(term);
}


bool TerminalWidget::setCellFont (const QFont &font)
{
	cell_font = font;
	cell_font.setFixedPitch(true);
	cell_font.setKerning(false);
	cell_font.setStyleHint(QFont::TypeWriter);

	QFontMetrics fm(font);
	cell_width  = fm.width('X');
	cell_height = fm.height();

	return true;
}


void TerminalWidget::processData (const QByteArray &data)
{
	Betty_ReceiveDataUTF8(term, (const unsigned char *)data.constData(), 
				data.count());	
}


/* XXX test */
int TerminalWidget::xmitData (void *xmit, const void *buf, unsigned int count)
{
	TerminalWidget *tw = (TerminalWidget *)xmit;
	return tw->pty->write(buf, count);
}


/* XXX test */
void TerminalWidget::execute ()
{	
	if (pty)
		return;
	
	
	pty = new Pty(this);

	Betty_SetTransmitFunction(term, this, TerminalWidget::xmitData);

	connect(pty, SIGNAL(dataReceived(QByteArray)),
		this, SLOT(processData(const QByteArray &)));
	
	/* start sh with --login to get /etc/profile loaded */
	static const char *argv[] = {
		"sh", "--login", NULL
	};

	pty->fork("/bin/sh", (char * const *)argv, (char * const *)environ);
	free(environ);
}


void TerminalWidget::setCursorVisible (bool on)
{
	if (cursor_visible != on) {
		cursor_visible = on;
		update(fromBettyPosition(cursor));
	}
}


/*--------------------------------------------------------------------------*/
/*	PAINT ENGINE							    */
/*--------------------------------------------------------------------------*/


struct Betty_Position TerminalWidget::toBettyPosition (int sx, int sy)
{
	struct Betty_Position pos;

	pos.x = sx + screen_delta.x;
	pos.y = sy + screen_delta.y;

	return pos;
}


/*
 * Return a rect in widget pixel coordinates, where the cell POS
 * is located.
 */
QRect TerminalWidget::fromBettyPosition (const struct Betty_Position &p)
{
	return QRect( 
		(p.x - screen_delta.x) * cell_width,
		(p.y - screen_delta.y) * cell_height,
		cell_width,
		cell_height
	);
}


QRect TerminalWidget::fromBettyRect (const struct Betty_Rect &r)
{
	return QRect(
		(r.left - screen_delta.x) * cell_width,
		(r.top  - screen_delta.y) * cell_height,
		(r.right - r.left + 1) * cell_width,
		(r.bottom - r.top + 1) * cell_height
	);
}


/*
 * Scrolls the cells in the scrolling range [TOP .. BOTTOM] COUNT
 * lines up/down. 
 *
 * COUNT < 0    scroll up
 * COUNT > 0    scroll down
 *
 */
void TerminalWidget::scrollCells (int top, int bottom, int count)
{
	int lines = bottom - top + 1;

	/* clip count */
	if (count < -lines)
		count = -lines;
	else if (count > lines)
		count = lines;

        if (abs(count) < lines) {
        	QRect r; 
		bool visible = cursor_visible;

		r.setLeft(0);
		r.setRight(width() - 1);
		r.setTop(top * cell_height);
		r.setBottom(bottom * cell_height + cell_height - 1);

		/* If the cursor is in the screen area that is scrolled,
		   we need to hide it first, otherwise we get cursor
		   artefacts */
		if (r.intersects(fromBettyPosition(cursor)))
			setCursorVisible(false);

		scroll(0, count * cell_height, r);
		setCursorVisible(visible);
	}

	/* redraw free space */
	QRect r;

	r.setLeft(0);
	r.setRight(width() - 1);

	if (count < 0) {
		r.setTop((bottom + count + 1) * cell_height);
		r.setBottom(bottom * cell_height + cell_height - 1);
	} else {
		r.setTop(top * cell_height);
		r.setBottom((top + count) * cell_height - 1);
	}

	update(r);	
}


QColor TerminalWidget::foregroundColor (const struct Betty_CellAttribute *attr) const
{
	unsigned int palette, index;

	if (!attr->reverse) {
		palette = attr->fg_palette;
		index = attr->fg_color_idx;
	} else {
		palette = attr->bg_palette;
		index = attr->bg_color_idx;
	}
	
	switch (palette) {
	case BETTY_PALETTE_DEFAULT:
		if (index >= sizeof(palette_default) / sizeof(*palette_default))
			break;

		if (!attr->bold)
			return palette_default[index];
		else
			return palette_default_light[index];

	case BETTY_PALETTE_XTERM:
                if (attr->bold && index < 8)
                        index += 8;

		if (index >= sizeof(palette_xterm) / sizeof(*palette_xterm))
			break;

		return palette_xterm[index];
	}

	return palette_default[BETTY_COLOR_DEFAULT_FG];
}


QColor TerminalWidget::backgroundColor (const struct Betty_CellAttribute *attr) const
{
	unsigned int palette, index;

	if (!attr->reverse) {
		palette = attr->bg_palette;
		index = attr->bg_color_idx;
	} else {
		palette = attr->fg_palette;
		index = attr->fg_color_idx;
	}

	switch (palette) {
	case BETTY_PALETTE_DEFAULT:
		if (index >= sizeof(palette_default) / sizeof(*palette_default))
			break;

		return palette_default[index];

	case BETTY_PALETTE_XTERM:
		if (index >= sizeof(palette_xterm) / sizeof(*palette_xterm))
			break;

		return palette_xterm[index];
	}

	return palette_default[BETTY_COLOR_DEFAULT_BG];
}


void TerminalWidget::paintOverlayCursor (QPainter &painter, const QRect &rect,
					CursorStyle cstyle)
{
        switch (cstyle) {
        case BlockCursor:
        	if (!hasFocus()) {
			painter.setPen(palette_default[BETTY_COLOR_DEFAULT_FG]);
			QRect r = rect;
			r.adjust(0, 0, 
				-painter.pen().width() - 1,
				-painter.pen().width() - 1);

                	painter.drawRect(r);
		}
                break;

	default:
		break;
        }
}


/*
 * Paints the background of COUNT cells starting at screen position
 * SX/SY.
 */
void TerminalWidget::paintBackground (QPainter &painter, 
					int sx, int sy, int count,
					const struct Betty_CellAttribute *attr,
					CursorStyle cstyle)
{
	QRect rect(
		sx * cell_width,
		sy * cell_height,
		count * cell_width,
		cell_height);

	if (cstyle == BlockCursor && hasFocus()) {
                painter.fillRect(rect, foregroundColor(attr));
                return;
        }
	
        painter.fillRect(rect, backgroundColor(attr));

	if (cstyle != NoCursor)
		paintOverlayCursor(painter, rect, cstyle);
}


/*
 * Paints the glyphs from the cell array CELL/COUNT to screen
 * starting at screen coordinates SX/SY.
 */
void TerminalWidget::paintGlyphs (QPainter &painter, int sx, int sy,
					const struct Betty_Cell *cell,
					int count, CursorStyle cstyle)
{
	QString str;
	const struct Betty_Cell *c;

	for (c = cell; c < cell + count; c++)
		str += QChar(c->uc);

	/* Get cell color. If we use a BlockCursor, this is a little
	   bit special. In opposite to overlay cursors, the BlockCursor
	   is a simple modification of fg/bg-color */
	QColor color;

        if (cstyle == BlockCursor && hasFocus())
	        color = backgroundColor(&cell->attr);
        else
	        color = foregroundColor(&cell->attr);

	QRect rect(
		sx * cell_width,
		sy * cell_height,
		cell_width * count,
		cell_height
	);

	/* paint glyphs */
	cell_font.setUnderline(cell->attr.underline);
	cell_font.setBold(cell->attr.bold);

	painter.setFont(cell_font);
	painter.setPen(color);
	painter.drawText(rect, Qt::TextSingleLine, str);

	if (cstyle != NoCursor)
		paintOverlayCursor(painter, rect, cstyle);
	
}


/*
 * determines the run length of same foreground attributes
 * in cell array CELL/COUNT.
 */
static inline int getFgAttrRun (const struct Betty_Cell *cell, int count)
{
	const struct Betty_Cell *c;

	if (count <= 0)
		return 0;

	for (c = cell + 1; c < cell + count; c++)
		if (cell->attr.fg_color_idx != c->attr.fg_color_idx ||
		    cell->attr.fg_palette   != c->attr.fg_palette   ||
		    cell->attr.underline    != c->attr.underline    ||
		    cell->attr.reverse      != c->attr.reverse	    ||
		    cell->attr.blink        != c->attr.blink	    ||
		    cell->attr.bold	    != c->attr.bold) 
		    	break;

	return c - cell;
}


/*
 * determines the run length of same background attributes
 * in cell array CELL/COUNT.
 */
static inline int getBgAttrRun (const struct Betty_Cell *cell, int count)
{
	const struct Betty_Cell *c;

	if (count <= 0)
		return 0;

	for (c = cell + 1; c < cell + count; c++)
		if (cell->attr.bg_color_idx != c->attr.bg_color_idx ||
		    cell->attr.bg_palette   != c->attr.bg_palette)
		    	break;

	return c - cell;
}


/*
 * Paints COUNT cells starting at screen position SX/SY to
 * screen.
 */
void TerminalWidget::paintLineSegment (QPainter &painter, 
					int sx, int sy, int count,
					CursorStyle cstyle)
{
	/* compute screen model coordinates and retrieve the
	   content */
	struct Betty_Position pos = toBettyPosition(sx, sy);
	const struct Betty_Line *line = Betty_GetLine(term, pos.y);

	/* If there is no content, simply paint empty cells */
	if (!line->cell || pos.x >= line->cell_count) {
		paintBackground(painter, sx, sy, count, 
				&BETTY_DEFAULT_CELL_ATTR, cstyle);
		return;
	}

	if (count > line->cell_count - pos.x) {
		int cell_count = line->cell_count - pos.x;

		paintBackground(painter, sx + cell_count, sy,
				count - cell_count, 
				&BETTY_DEFAULT_CELL_ATTR, cstyle);
	}

	/* compute cell array to use */
	const struct Betty_Cell *cell = line->cell + pos.x;
	int cell_count = line->cell_count - pos.x;

	if (cell_count > count)
		cell_count = count;

	/* paint background */
	const struct Betty_Cell *c = cell;
	int rl, clen = cell_count;

	while (clen > 0 && (rl = getBgAttrRun(c, clen)) > 0) {
		paintBackground(painter, sx + c - cell, sy, rl,
				&c->attr, cstyle);
		c += rl;
		clen -= rl;
	}

	/* paint glyphs */
	c = cell;
	clen = cell_count;

	while (clen > 0 && (rl = getFgAttrRun(c, clen)) > 0) {
		paintGlyphs(painter, sx + c - cell, sy, c, rl, cstyle);
		
		c += rl;
		clen -= rl;
	}
	
}


/*
 * Paints the rect RECT to screen.
 *
 * Because of the painting of text is expensive we do it in a special
 * way. We try to accumulate as much characters as possible per draw
 * operation.
 */
void TerminalWidget::paintRect (QPainter &painter, const QRect &rect)
{
	int sx1, sy1, sx2, sy2, csx2, csy2;

	sx1 = rect.left()   / cell_width;
	sx2 = rect.right()  / cell_width;
	sy1 = rect.top()    / cell_height;
	sy2 = rect.bottom() / cell_height;

	csx2 = qMin(sx2, Betty_GetWidth(term) - 1);
	csy2 = qMin(sy2, Betty_GetHeight(term) - 1);

	for (int sy = sy1; sy <= csy2; sy++)
		paintLineSegment(painter, sx1, sy, csx2 - sx1 + 1);

	/* paint cursor */
	int scx = cursor.x - screen_delta.x;
	int scy = cursor.y - screen_delta.y;

	if (cursor_visible &&
	    scx >= sx1 && scx <= csx2 &&
	    scy >= sy1 && scy <= csy2)
		paintLineSegment(painter, scx, scy, 1, cursor_style);

	/* paint right padding */
	if (sx2 > csx2) {
		QRect rect(
			(csx2 + 1) * cell_width,
			sy1 * cell_height,
			(sx2 - csx2) * cell_width,
			(csy2 - sy1 + 1) * cell_height
		);

		painter.fillRect(rect, Qt::gray);
	}

	/* paint bottom padding */
	if (sy2 > csy2) {
		QRect rect(
			0,
			(csy2 + 1) * cell_height,
			width(),
			(sy2 - csy2) * cell_height
		);

		painter.fillRect(rect, Qt::gray);
	}
}


void TerminalWidget::paintEvent (QPaintEvent *event)
{
	QPainter painter(this);

	foreach (const QRect &rect, event->region().rects()) {
		paintRect(painter, rect);
	}
}


void TerminalWidget::keyPressEvent (QKeyEvent *event)
{
	if (event->text().isEmpty()) {
		const char *data = "";
		switch (event->key()) {
		case Qt::Key_Backspace:
			data = "\x7f";
			Betty_SendString(term, data);
			break;

		case Qt::Key_Up:
			data = "\033[A";
			Betty_SendString(term, data);
			break;

		case Qt::Key_Down:
			data = "\033[B";
			Betty_SendString(term, data);
			break;

		case Qt::Key_Left:
			data = "\033[D";
			Betty_SendString(term, data);
			break;

		case Qt::Key_Right:
			data = "\033[C";
			Betty_SendString(term, data);
			break;
		default:
			/* ctrl-... key events generated in my boox terminal emulator do not have a text field */
			char c=(char)(event->key());
			// Betty_SendString(term,"%c",(char)(event->key()));
			Betty_SendData (term, &c,1);
		}
		return;
	}
	{ /* BP: I was missing enters and backspace in the onyx, they were sent to the terminal but did not turn up there. */
	  /*     aparently the backspace and enter send from the onux keyboard have a non empty text field?? */
	const char *data = "";
	switch (event->key()) {
	case Qt::Key_Enter:
		data = "\n";
		Betty_SendString(term, data);
		return;
	case Qt::Key_Backspace:
		data = "\x7f";
		Betty_SendString(term, data);
		return;
	}
	}
	// printf("%i\n", (int) (event->key()));
	QByteArray data = event->text().toLocal8Bit();
	Betty_SendData(term, data.constData(), data.count());
}


/*--------------------------------------------------------------------------*/
/*      libbetty bindings                                                   */
/*--------------------------------------------------------------------------*/


void TerminalWidget::uiScrollUp (void *ui, int top, int bottom, int count)
{
	TerminalWidget *tw = (TerminalWidget *)ui;
	tw->scrollCells(top, bottom, -count);
}

void TerminalWidget::uiScrollDown (void *ui, int top, int bottom, int count)
{
	TerminalWidget *tw = (TerminalWidget *)ui;
	tw->scrollCells(top, bottom, count);
}

void TerminalWidget::uiDrawCell (void *ui, struct Betty_Position pos, 
                        const struct Betty_Cell *)
{
	TerminalWidget *tw = (TerminalWidget *)ui;
	tw->update(tw->fromBettyPosition(pos));
}

void TerminalWidget::uiDrawRect (void *ui, struct Betty_Rect rect)
{
	TerminalWidget *tw = (TerminalWidget *)ui;
	tw->update(tw->fromBettyRect(rect));
}

void TerminalWidget::uiMoveCursor (void *ui, struct Betty_Position pos_new)
{
	TerminalWidget *tw = (TerminalWidget *)ui;

	if (pos_new.x == tw->cursor.x && pos_new.y == tw->cursor.y)
		return;

	struct Betty_Position pos_old = tw->cursor;
	tw->cursor = pos_new;

	tw->update(tw->fromBettyPosition(pos_old));
	tw->update(tw->fromBettyPosition(pos_new));
}

void TerminalWidget::uiBeep (void *)
{
	QApplication::beep();
}

void TerminalWidget::uiResizeScreen (void *ui, int, int)
{
	TerminalWidget *tw = (TerminalWidget *)ui;
	
	tw->update();
}


const struct Betty_UiOperations TerminalWidget::ui_ops = {
        // .scrollUp =
        TerminalWidget::uiScrollUp,
        // .scrollDown =
        TerminalWidget::uiScrollDown,
        // .drawCell =
        TerminalWidget::uiDrawCell,
        // .drawRect =
        TerminalWidget::uiDrawRect,
        // .moveCursor =
        TerminalWidget::uiMoveCursor,
        // .bell =
	TerminalWidget::uiBeep,
	// .resizeScreen =
	TerminalWidget::uiResizeScreen
};



