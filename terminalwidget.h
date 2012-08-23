
#ifndef __TERMINALWIDGET_H__
#define __TERMINALWIDGET_H__

#include <QWidget>
#include <QFont>
#include <QPaintEvent>
#include <QKeyEvent>

#include <betty/terminal.h>
#include "pty.h"


class TerminalWidget : public QWidget
{
	Q_OBJECT

public:
        enum CursorStyle {
                NoCursor = 0,
                BlockCursor
        };

	TerminalWidget (QWidget *parent = 0, Qt::WindowFlags f = 0);
	~TerminalWidget ();

	bool setCellFont (const QFont &font);
	void execute ();

public slots:

	void processData (const QByteArray &data);
	void setCursorVisible (bool on);

protected:

	struct Betty_Position toBettyPosition (int sx, int sy);
	QRect fromBettyPosition (const struct Betty_Position &pos);
	QRect fromBettyRect (const struct Betty_Rect &r);

        void scrollCells (int top, int bottom, int count);

	QColor foregroundColor (const struct Betty_CellAttribute *attr) const;
	QColor backgroundColor (const struct Betty_CellAttribute *attr) const;

	void paintOverlayCursor (QPainter &painter, const QRect &rect, 
				CursorStyle cstyle);
	void paintBackground (QPainter &painter, int sx, int sy, int count,
				const struct Betty_CellAttribute *attr,
				CursorStyle cstyle = NoCursor);
	void paintGlyphs (QPainter &painter, int sx, int sy,
				const struct Betty_Cell *cell, int count, 
				CursorStyle cstyle = NoCursor);
	void paintLineSegment (QPainter &painter, int sx, int sy, int count,
				CursorStyle cstyle = NoCursor);
      	void paintRect (QPainter &painter, const QRect &rect);
	void paintEvent (QPaintEvent *event);

	void keyPressEvent (QKeyEvent *event);
	

private:

	struct Betty_Terminal *term;
        struct Betty_Position cursor;
	struct Betty_Position screen_delta;

	/* cell properties */
	QFont		cell_font;
	int		cell_width;
	int		cell_height;

	CursorStyle	cursor_style;
	bool		cursor_visible;

	Pty		*pty;

	/* betty library bindings */
	static const struct Betty_UiOperations ui_ops;

	static void uiScrollUp (void *ui, int top, int bottom, int count);
	static void uiScrollDown (void *ui, int top, int bottom, int count);
	static void uiDrawCell (void *ui, struct Betty_Position pos, const struct Betty_Cell *);
	static void uiDrawRect (void *ui, struct Betty_Rect rect);
	static void uiMoveCursor (void *ui, struct Betty_Position pos_new);
	static void uiBeep (void *);
	static void uiResizeScreen (void *ui, int, int);

	static int xmitData (void *xmit, const void *buf, unsigned int count);
};


#endif

