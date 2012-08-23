#ifndef __MAINWINDOW_H__
#define __MAINWINDOW_H__
#include <onyx/ui/buttons.h>
#include <onyx/ui/onyx_dialog.h>
#include <onyx/ui/keyboard.h>
#include "terminalwidget.h"

namespace ui
{

class BooxTerminal : public OnyxDialog
{
    Q_OBJECT

public:
    BooxTerminal(QWidget *parent);
    ~BooxTerminal(void);

public:
    int popup();

protected:
    void mouseMoveEvent(QMouseEvent *me);
    void mousePressEvent(QMouseEvent *me);
    void mouseReleaseEvent(QMouseEvent *me);
    void keyReleaseEvent(QKeyEvent *);
    void keyPressEvent(QKeyEvent * ke);
    bool event(QEvent * event);
    void moveEvent(QMoveEvent *e);
    void resizeEvent(QResizeEvent *e);
    // bool eventFilter(QObject *obj, QEvent *event);

private Q_SLOTS:
    void emitDel();
    void emitCtrlC();
    void emitEsc();
    void SetCtrlSeq();
    void onCloseClicked();

private:
    void createLayout();
     QGroupBox *gridbox;
    
    QPushButton *b_del;
    QPushButton *b_ctrc;
    QPushButton *b_esc;
    QPushButton *b_ctr_;
    
    
    TerminalWidget term;

    KeyBoard      keyboard_;     ///< Keyboard.
    bool	  ctrl_;
};


};  // namespace ui

#endif  

