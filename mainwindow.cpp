/*
 * mainwindow.cpp
 *
 * Main window for the betty terminal emulator for the boox m92
 *
 *
 * 23.08.2012 Bart
 */
#include <onyx/screen/screen_proxy.h>
#include <onyx/ui/keyboard_navigator.h>
#include "/opt/onyx/arm/include/onyx/screen/screen_update_watcher.h"
#include <QGridLayout> 
#include <QPushButton>

#include "mainwindow.h"

namespace ui
{
BooxTerminal::BooxTerminal(QWidget *parent)
    : OnyxDialog(parent)
    , term(this)
    , keyboard_(0)
{
    createLayout();

    // Widget attributes.
    setModal(true);
    // setFocusPolicy(Qt::NoFocus);

    updateTitle("Betty Boox Terminal Emulator");
    
    term.execute();
    ctrl_=false;

}

BooxTerminal::~BooxTerminal(void)
{
}

void BooxTerminal::createLayout()
{
    title_icon_label_.setPixmap(QPixmap(":/images/betty_boox.png"));
    gridbox = new QGroupBox(tr(""));
     QGridLayout *layout = new QGridLayout;


     b_del = new QPushButton(tr("DEL"),this);
     b_ctrc = new QPushButton(tr("Ctr-C"),this);
     b_esc = new QPushButton(tr("ESC"),this);     
     b_ctr_ = new QPushButton(tr("Ctr-..."), this);
     
     b_del->setFlat(true);
     b_ctrc->setFlat(true);
     b_esc->setFlat(true);
     b_ctr_->setFlat(true);
     
     b_del->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding);
     b_ctrc->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding);
     b_esc->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding);
     b_ctr_->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding);
     b_ctr_->setCheckable(true);
     
     layout->addWidget(b_del,0,0);
     layout->addWidget(b_ctrc,1,0);
     layout->addWidget(b_esc,2,0);
     layout->addWidget(b_ctr_,3,0);
     
    // terminal.
    term.setFixedHeight(410);
    
     layout->addWidget(&term,0,1,4,-1);
 
    // keyboard.
    keyboard_.attachReceiver(this);
    layout->addWidget(&keyboard_,4,0,-1,-1);
     layout->setRowStretch(4, 10);
     layout->setColumnStretch(1, 10);
     gridbox->setLayout(layout);
     vbox_.addWidget(gridbox);
    
    connect( b_del, SIGNAL(clicked()), this, SLOT(emitDel()) );
    connect( b_ctrc, SIGNAL(clicked()), this, SLOT(emitCtrlC()) );
    connect( b_esc, SIGNAL(clicked()), this, SLOT(emitEsc()) );
    connect( b_ctr_, SIGNAL(clicked()), this, SLOT(SetCtrlSeq()) );
    keyboard_.installEventFilter(this);
}

void BooxTerminal::onCloseClicked()
{
    shadows_.show(false);
    done(QDialog::Rejected);
}


void BooxTerminal::mouseMoveEvent(QMouseEvent *me)
{
    me->accept();
}

void BooxTerminal::mousePressEvent(QMouseEvent *me)
{
    me->accept();
    
}

void BooxTerminal::mouseReleaseEvent(QMouseEvent *me)
{
    me->accept();
}

void BooxTerminal::keyReleaseEvent(QKeyEvent *ke)
{
    int key = ke->key();
    if (key == Qt::Key_Escape)
    {
        onCloseClicked();
        return;
    }
}

void BooxTerminal::emitDel()
{
    	QKeyEvent key(QKeyEvent::KeyPress, 0x7f, Qt::NoModifier, "", false, 0 );
        QApplication::sendEvent(&term, &key);
	return;
}
void BooxTerminal::emitCtrlC()
{
    	QKeyEvent key(QKeyEvent::KeyPress, 0x03, Qt::NoModifier, "", false, 0 );
        QApplication::sendEvent(&term, &key);
	return;
}
void BooxTerminal::emitEsc()
{
    	QKeyEvent key(QKeyEvent::KeyPress, 0x1B, Qt::NoModifier, "", false, 0 );
        QApplication::sendEvent(&term, &key);
	return;
}
void BooxTerminal::SetCtrlSeq()
{
    if (!ctrl_)
    {
    	ctrl_=true;
    	b_ctr_->setChecked(true);
    }
    else
    {
    	ctrl_=false;
    	b_ctr_->setChecked(false);
    }
    onyx::screen::instance().updateWidget(b_ctr_, onyx::screen::ScreenProxy::GU);
    return;
}

/* every key event will be forwarded to the terminal, here we check for modifier keys etc. */
void BooxTerminal::keyPressEvent(QKeyEvent * ke)
{
    ke->accept();
    if (ke->key() == Qt::Key_Shift || ke->key() == Qt::Key_CapsLock)
    {
        return;
    }
    
    if ((ctrl_==true)&&(ke->key()>=0x40)&&(ke->key()<=0x5f))
    {	
   	QKeyEvent key(QKeyEvent::KeyPress, ke->key()-0x40, Qt::NoModifier, "", false, 0 );
       	QApplication::sendEvent(&term, &key);
    }
    else if ((ctrl_==true)&&(ke->key()>=0x61)&&(ke->key()<=0x7a))
    {	
	QKeyEvent key(QKeyEvent::KeyPress, ke->key()-0x60, Qt::NoModifier, "", false, 0 );
       	QApplication::sendEvent(&term, &key);
    }	
    else
    	QApplication::sendEvent(&term, ke);
    
      	
    // Disable the parent widget to update screen.
    onyx::screen::instance().enableUpdate(false);

   while (QApplication::hasPendingEvents())
    {
        QApplication::processEvents();
    }
    onyx::screen::instance().enableUpdate(true);
    if (ctrl_)
    {
        ctrl_=false;
        b_ctr_->setChecked(false);
        onyx::screen::instance().updateWidget(b_ctr_, onyx::screen::ScreenProxy::GU); 
        // onyx::screen::instance().updateWidget(&term, onyx::screen::ScreenProxy::GU);
    } 
    onyx::screen::instance().updateWidget(this, onyx::screen::ScreenProxy::GU);
}

bool BooxTerminal::event(QEvent * event)
{
    bool ret = OnyxDialog::event(event);
    if (event->type() == QEvent::UpdateRequest)
    {
        // onyx::screen::instance().sync(&shadows_.hor_shadow());
        // onyx::screen::instance().sync(&shadows_.ver_shadow());
        onyx::screen::instance().updateWidget(this, onyx::screen::ScreenProxy::GU);
        // onyx::screen::instance().updateWidget(this, onyx::screen::ScreenProxy::GU);
    }
    return ret;
    
}

void BooxTerminal::moveEvent(QMoveEvent *e)
{
    OnyxDialog::moveEvent(e);
}

void BooxTerminal::resizeEvent(QResizeEvent *e)
{
    OnyxDialog::resizeEvent(e);    
}

}   // namespace ui




