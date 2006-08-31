/*****************************************************************************/
/*                                                                           */
/* This program is free software; you can redistribute it and/or modify it   */
/* under the terms of the GNU General Public License as published by the     */ 
/* Free Software Foundation; version 2 of the License.	                     */
/*                                                                           */
/* This program is distributed in the hope that it will be useful, but       */
/* WITHOUT ANY WARRANTY; without even the implied warranty of                */ 
/* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General  */
/* Public License for more details.                                          */
/*                                                                           */
/* You should have received a copy of the GNU General Public License along   */
/* with this program; (See "LICENSE.GPL"). If not, write to the Free         */
/* Software Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA        */
/* 02111-1307, USA.                                                          */
/*                                                                           */
/*---------------------------------------------------------------------------*/
/*                                                                           */
/*      Reinhard Katzmann, GERMANY                                           */
/*      reinhard@suamor.de                                                   */
/*                                                                           */
/*      Matevž Jekovec, SLOVENIA                                             */
/*      matevz.jekovec@gmail.com                                             */
/*                                                                           */
/*****************************************************************************/

#include <QtGui/QtGui>
#include <QSlider>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QPoint>
#include <QKeyEvent>
#include <QString>
#include <QTextStream>
#include <QXmlInputSource>
#include <iostream>

using namespace std;

#include "ui/mainwin.h"
#include "ui/keysigpsp.h"

#include "widgets/toolbar.h"
#include "widgets/lcdnumber.h"

#include "interface/rtmididevice.h"
#include "interface/playback.h"
#include "interface/engraver.h"

#include "widgets/scrollwidget.h"
#include "widgets/viewport.h"
#include "widgets/scoreviewport.h"
#include "widgets/sourceviewport.h"

#include "drawable/drawablecontext.h"
#include "drawable/drawablestaff.h"
#include "drawable/drawablemuselement.h"
#include "drawable/drawablenote.h"
#include "drawable/drawableaccidental.h"	//DEBUG, this isn't needed though

#include "core/sheet.h"
#include "core/staff.h"
#include "core/clef.h"
#include "core/keysignature.h"
#include "core/note.h"
#include "core/canorusml.h"
#include "core/voice.h"
#include "core/barline.h"

// Constructor
CAMainWin::CAMainWin(QMainWindow *oParent)
  : QMainWindow( oParent )
{
	mpoMEToolBar = new CAToolBar( this );
	mpoMEToolBar->setOrientation(Qt::Vertical);
	initToolBar();
	addToolBar(static_cast<Qt::ToolBarArea>(2), mpoMEToolBar);
	
	//Initialize widgets
	moMainWin.setupUi( this );
	// Add a signal so if the menu action is used, the toolbar
	// is notified to change it's state
	mpoMEToolBar->setAction( actionClefSelect->objectName(), 
	                         moMainWin.action_Clef );
	mpoVoiceNum = new CALCDNumber( 0, 20, 0, "Voice number" );
	mpoVoiceNumAction = moMainWin.mpoToolBar->addWidget( mpoVoiceNum );
	// Connect manually as the action cannot be created earlier
	connect( mpoVoiceNum, SIGNAL( valChanged( int ) ), this,
	         SLOT( sl_mpoVoiceNum_valChanged( int ) ) );

	moMainWin.actionAnimated_scroll->setChecked(true);
	moMainWin.actionLock_scroll_playback->setChecked(false);
	moMainWin.actionUnsplit->setEnabled(false);
	
	mpoKeySigPSP = 0;
	
	//Initialize MIDI
	initMidi();
	
	//Initialize the internal properties
	_currentMode = SelectMode;
	_insertMusElement = CAMusElement::None;
	_insertNote       = CANote::Quarter;
	_insertClef       = CAClef::Treble;
	_playback = 0;
	_animatedScroll = true;
	_lockScrollPlayback = false;
	
	newDocument();
}

CAMainWin::~CAMainWin() 
{
	delete mpoClefMenu;
	delete mpoNoteMenu;
	delete mpoKeySigMenu;
	delete _midiOut;
	delete mpoMEToolBar;
}

void CAMainWin::initToolBar()
{
	// Test Code for Toolbar
	mpoClefMenu   = new CAButtonMenu( tr("Select Clef" ) );
	mpoNoteMenu   = new CAButtonMenu( tr("Select Note" ) );
	mpoKeySigMenu = new QMenu(); 
	
	QIcon oClefTrebleIcon( QString::fromUtf8(":/menu/images/cleftreble.png") );
	QIcon oClefBassIcon(   QString::fromUtf8(":/menu/images/clefbass.png") );
	QIcon oClefAltoIcon(   QString::fromUtf8(":/menu/images/clefalto.png") );
	
	QIcon oN0Icon(  QString::fromUtf8(":/menu/images/n0.png") );
	QIcon oN1Icon(  QString::fromUtf8(":/menu/images/n1.png") );
	QIcon oN2Icon(  QString::fromUtf8(":/menu/images/n2.png") );
	QIcon oN4Icon(  QString::fromUtf8(":/menu/images/n4.png") );
	QIcon oN8Icon(  QString::fromUtf8(":/menu/images/n8.png") );
	QIcon oN16Icon(  QString::fromUtf8(":/menu/images/n16.png") );
	QIcon oN32Icon(  QString::fromUtf8(":/menu/images/n32.png") );
	QIcon oN64Icon(  QString::fromUtf8(":/menu/images/n64.png") );
	
	actionClefSelect = mpoMEToolBar->addToolMenu( "Add Clef", "actionClefSelect",
	                                              mpoClefMenu, 
	                                              &oClefTrebleIcon, true );
	actionNoteSelect = mpoMEToolBar->addToolMenu( "Change Note length",
	                                              "actionNoteSelect", mpoNoteMenu,
	                                              &oN4Icon, true );
	QIcon oDFIcon( QString::fromUtf8(":/menu/images/doubleflat.png") );
	actionKeySigSelect = mpoMEToolBar->addToolMenu( "Add Key Signature",
	                                                "actionKeySigSelect", mpoKeySigMenu,
	                                                &oDFIcon, true );	
	// Add all the menu entries, either as text or icons
	mpoClefMenu->addButton( oClefTrebleIcon, CAClef::Treble );
	mpoClefMenu->addButton( oClefBassIcon, CAClef::Bass );
	mpoClefMenu->addButton( oClefAltoIcon, CAClef::Alto );
	mpoNoteMenu->addButton( oN0Icon, CANote::Breve );
	mpoNoteMenu->addButton( oN1Icon, CANote::Whole );
	mpoNoteMenu->addButton( oN2Icon, CANote::Half );
	mpoNoteMenu->addButton( oN4Icon, CANote::Quarter );
	mpoNoteMenu->addButton( oN8Icon, CANote::Eighth );
	mpoNoteMenu->addButton( oN16Icon, CANote::Sixteenth );
	mpoNoteMenu->addButton( oN32Icon, CANote::ThirtySecond );
	mpoNoteMenu->addButton( oN64Icon, CANote::SixtyFourth );
	mpoKeySigMenu->addAction( "C" );
	mpoKeySigMenu->addAction( "G" );
	mpoKeySigMenu->addAction( "D" );
	mpoKeySigMenu->addAction( "A" );
	mpoKeySigMenu->addAction( "E" );
}

void CAMainWin::newDocument() {
	_document.clear();	//clear the logical part
	clearUI();			//clear the UI part
	
	addSheet(_document.addSheet(QString("Sheet ") + QString::number(_document.sheetCount()+1)));			//add a new empty sheet
	on_actionNew_staff_activated();	//add a new empty 5-line staff to the sheet
}

void CAMainWin::addSheet(CASheet *s) {
	CAScoreViewPort *v = new CAScoreViewPort(s, 0);
	v->setWindowIcon(QIcon(QString::fromUtf8(":/menu/images/clogosm.png")));
	connect(v, SIGNAL(CAMousePressEvent(QMouseEvent *, QPoint, CAViewPort *)), this, SLOT(viewPortMousePressEvent(QMouseEvent *, QPoint, CAViewPort *)));
	connect(v, SIGNAL(CAMouseMoveEvent(QMouseEvent *, QPoint, CAViewPort *)), this, SLOT(viewPortMouseMoveEvent(QMouseEvent *, QPoint, CAViewPort *)));
	connect(v, SIGNAL(CAWheelEvent(QWheelEvent *, QPoint, CAViewPort *)), this, SLOT(viewPortWheelEvent(QWheelEvent *, QPoint, CAViewPort *)));
	connect(v, SIGNAL(CAKeyPressEvent(QKeyEvent *, CAViewPort *)), this, SLOT(viewPortKeyPressEvent(QKeyEvent *, CAViewPort *)));
	v->setFocusPolicy(Qt::ClickFocus);
	v->setFocus();

	_viewPortList.append(v);
	
	moMainWin.tabWidget->addTab(new CAScrollWidget(v, 0), s->name());
	moMainWin.tabWidget->setCurrentIndex(moMainWin.tabWidget->count()-1);
	
	_activeViewPort = v;
}

void CAMainWin::clearUI() {
	for (int i=0; i<_viewPortList.size(); i++)
		delete _viewPortList[i];

	_viewPortList.clear();
	
	while (moMainWin.tabWidget->count()) {
		delete _currentScrollWidget;
		moMainWin.tabWidget->removeTab(moMainWin.tabWidget->currentIndex());
	}
	
	_fileName = "";
}

void CAMainWin::on_tabWidget_currentChanged(int idx) {
	_activeViewPort = _currentScrollWidget->lastUsedViewPort();
}

void CAMainWin::on_action_Fullscreen_toggled(bool checked) {
	if (checked)
		this->showFullScreen();
	else
		this->showNormal();
}

void CAMainWin::on_actionSplit_horizontally_activated() {
	CAViewPort *v = (CAViewPort *)_currentScrollWidget->splitHorizontally();
	v->setWindowIcon(QIcon(QString::fromUtf8(":/menu/images/clogosm.png")));
	if (v->viewPortType() == CAViewPort::ScoreViewPort) {
		connect((CAScoreViewPort*)v, SIGNAL(CAMousePressEvent(QMouseEvent *, QPoint, CAViewPort *)), this, SLOT(viewPortMousePressEvent(QMouseEvent *, QPoint, CAViewPort *)));
		connect((CAScoreViewPort*)v, SIGNAL(CAMouseMoveEvent(QMouseEvent *, QPoint, CAViewPort *)), this, SLOT(viewPortMouseMoveEvent(QMouseEvent *, QPoint, CAViewPort *)));
		connect((CAScoreViewPort*)v, SIGNAL(CAWheelEvent(QWheelEvent *, QPoint, CAViewPort *)), this, SLOT(viewPortWheelEvent(QWheelEvent *, QPoint, CAViewPort *)));
		connect((CAScoreViewPort*)v, SIGNAL(CAKeyPressEvent(QKeyEvent *, CAViewPort *)), this, SLOT(viewPortKeyPressEvent(QKeyEvent *, CAViewPort *)));
		v->setFocusPolicy(Qt::ClickFocus);
		v->setFocus();
	}
	
	moMainWin.actionSplit_horizontally->setEnabled(false);
	moMainWin.actionSplit_vertically->setEnabled(false);
	moMainWin.actionUnsplit->setEnabled(true);
	_viewPortList.append(v);
	setMode(_currentMode);	//updates the new viewport border settings
}

void CAMainWin::on_actionSplit_vertically_activated() {
	CAViewPort *v = (CAViewPort *)_currentScrollWidget->splitVertically();

	v->setWindowIcon(QIcon(QString::fromUtf8(":/menu/images/clogosm.png")));
	if (v->viewPortType() == CAViewPort::ScoreViewPort) {
		connect((CAScoreViewPort*)v, SIGNAL(CAMousePressEvent(QMouseEvent *, QPoint, CAViewPort *)), this, SLOT(viewPortMousePressEvent(QMouseEvent *, QPoint, CAViewPort *)));
		connect((CAScoreViewPort*)v, SIGNAL(CAMouseMoveEvent(QMouseEvent *, QPoint, CAViewPort *)), this, SLOT(viewPortMouseMoveEvent(QMouseEvent *, QPoint, CAViewPort *)));
		connect((CAScoreViewPort*)v, SIGNAL(CAWheelEvent(QWheelEvent *, QPoint, CAViewPort *)), this, SLOT(viewPortWheelEvent(QWheelEvent *, QPoint, CAViewPort *)));
		connect((CAScoreViewPort*)v, SIGNAL(CAKeyPressEvent(QKeyEvent *, CAViewPort *)), this, SLOT(viewPortKeyPressEvent(QKeyEvent *, CAViewPort *)));
		v->setFocusPolicy(Qt::ClickFocus);
		v->setFocus();
	}
	
	moMainWin.actionSplit_horizontally->setEnabled(false);
	moMainWin.actionSplit_vertically->setEnabled(false);
	moMainWin.actionUnsplit->setEnabled(true);
	_viewPortList.append(v);
	setMode(_currentMode);	//updates the new viewport border settings
}

void CAMainWin::on_actionUnsplit_activated() {
	CAViewPort *v = _currentScrollWidget->unsplit();
	if (v)
		_viewPortList.removeAll(v);
	
	moMainWin.actionSplit_horizontally->setEnabled(true);
	moMainWin.actionSplit_vertically->setEnabled(true);
	moMainWin.actionUnsplit->setEnabled(false);
}

void CAMainWin::on_actionSource_view_perspective_activated() {
	CASourceViewPort *v = new CASourceViewPort(&_document, _activeViewPort->parent());
	_currentScrollWidget->addViewPort(v);
	
	connect(v, SIGNAL(CACommit(QString)), this, SLOT(sourceViewPortCommit(QString)));
	
	v->setWindowIcon(QIcon(QString::fromUtf8(":/menu/images/clogosm.png")));
	v->setFocusPolicy(Qt::ClickFocus);
	v->setFocus();
	
	moMainWin.actionSplit_horizontally->setEnabled(false);
	moMainWin.actionSplit_vertically->setEnabled(false);
	moMainWin.actionUnsplit->setEnabled(true);
	_viewPortList.append(v);
	setMode(_currentMode);	//updates the new viewport border settings
}

void CAMainWin::on_actionNew_viewport_activated() {
	CAViewPort *v = _currentScrollWidget->newViewPort(_activeViewPort);

	v->setWindowIcon(QIcon(QString::fromUtf8(":/menu/images/clogosm.png")));
	if (v->viewPortType() == CAViewPort::ScoreViewPort) {
		connect((CAScoreViewPort*)v, SIGNAL(CAMousePressEvent(QMouseEvent *, QPoint, CAViewPort *)), this, SLOT(viewPortMousePressEvent(QMouseEvent *, QPoint, CAViewPort *)));
		connect((CAScoreViewPort*)v, SIGNAL(CAMouseMoveEvent(QMouseEvent *, QPoint, CAViewPort *)), this, SLOT(viewPortMouseMoveEvent(QMouseEvent *, QPoint, CAViewPort *)));
		connect((CAScoreViewPort*)v, SIGNAL(CAWheelEvent(QWheelEvent *, QPoint, CAViewPort *)), this, SLOT(viewPortWheelEvent(QWheelEvent *, QPoint, CAViewPort *)));
		connect((CAScoreViewPort*)v, SIGNAL(CAKeyPressEvent(QKeyEvent *, CAViewPort *)), this, SLOT(viewPortKeyPressEvent(QKeyEvent *, CAViewPort *)));
		v->setFocusPolicy(Qt::ClickFocus);
		v->setFocus();
	}

	_viewPortList.append(v);
	setMode(_currentMode);	//updates the new viewport border settings
}

void CAMainWin::on_actionNew_activated() {
	newDocument();
}

void CAMainWin::on_actionNew_sheet_activated() {
	addSheet(_document.addSheet(QString("Sheet ") + QString::number(_document.sheetCount()+1)));			//add a new empty sheet
}

void CAMainWin::on_actionNew_staff_activated() {
	if (_activeViewPort->viewPortType() != CAViewPort::ScoreViewPort)
		return;
	
	CASheet *sheet = ((CAScoreViewPort*)_activeViewPort)->sheet();
	CAStaff *staff = sheet->addStaff();
	staff->addVoice(new CAVoice(staff, QString("Voice ") + QString::number(staff->voiceCount()+1)));
	
	rebuildViewPorts(sheet);
	
	((CAScoreViewPort*)_activeViewPort)->selectContext(staff);
	((CAScoreViewPort*)_activeViewPort)->repaint();
}

void CAMainWin::setMode(CAMode mode) {
	_currentMode = mode;
	
	QPen p;
	switch (mode) {
		case SelectMode:
			for (int i=0; i<_viewPortList.size(); i++) {
				if (_viewPortList[i]->viewPortType()==CAViewPort::ScoreViewPort) {
					if (!((CAScoreViewPort*)_viewPortList[i])->playing())
						((CAScoreViewPort*)_viewPortList[i])->unsetBorder();
					((CAScoreViewPort*)_viewPortList[i])->setShadowNoteVisible(false);
					statusBar()->showMessage("");
					_insertMusElement = CAMusElement::None;
					((CAScoreViewPort*)_viewPortList[i])->repaint();
				}
			}
			break;
		case InsertMode:
			p.setColor(Qt::blue);
			p.setWidth(3);
			
			if (_insertMusElement == CAMusElement::Note)
				((CAScoreViewPort*)_activeViewPort)->setShadowNoteVisible(true);

			for (int i=0; i<_viewPortList.size(); i++) {
				if (_viewPortList[i]->viewPortType()==CAViewPort::ScoreViewPort) {
					if (!((CAScoreViewPort*)_viewPortList[i])->playing())
						((CAScoreViewPort*)_viewPortList[i])->setBorder(p);
					((CAScoreViewPort*)_viewPortList[i])->repaint();
				}
			}

			break;
	}
}

void CAMainWin::on_action_Clef_activated() {
	setMode(InsertMode);
	_insertMusElement = CAMusElement::Clef;
}

void CAMainWin::rebuildViewPorts(CASheet *sheet, bool repaint) {
	for (int i=0; i<_viewPortList.size(); i++) {
		_viewPortList[i]->rebuild();
		
		if (_viewPortList[i]->viewPortType() == CAViewPort::ScoreViewPort)
			((CAScoreViewPort*)(_viewPortList[i]))->checkScrollBars();
			
		_viewPortList[i]->repaint();
	}
}

void CAMainWin::viewPortMousePressEvent(QMouseEvent *e, const QPoint coords, CAViewPort *viewPort) {
	_activeViewPort = viewPort;
	
	if (viewPort->viewPortType() == CAViewPort::ScoreViewPort) {
		CAScoreViewPort *v = (CAScoreViewPort*)viewPort;
		
		if (e->modifiers()==Qt::ControlModifier) {
			CAMusElement *elt;
			if ( elt = v->removeMElement(coords.x(), coords.y()) ) {
				delete elt;
				rebuildViewPorts(v->sheet());
				v->repaint();
				return;
			}
		}
		
		switch (_currentMode) {
			case SelectMode:
				if ( v->selectMElement(coords.x(), coords.y()) ||
				     v->selectCElement(coords.x(), coords.y()) )
					v->repaint();
				break;
			
			case InsertMode:
				insertMusElementAt( coords, v );
				break;
		}
	}
}

void CAMainWin::insertMusElementAt(const QPoint coords, CAScoreViewPort* v) {
	CADrawableContext *context = v->selectCElement(coords.x(), coords.y());
	CAMusElement *right = v->nearestRightElement(coords.x(), coords.y());
	
	CAStaff *staff=0;
	CADrawableStaff *drawableStaff=0;
	CAClef *clef=0;
	CANote *note=0;
	switch (_insertMusElement) {
		case CAMusElement::Clef:
			if ( (!context) ||
			    (context->context()->contextType() != CAContext::Staff) )
				return;
			
			staff = (CAStaff*)context->context();
			clef = new CAClef(_insertClef, staff, (right?right->timeStart():staff->lastTimeEnd()));
			staff->insertSignBefore(clef, right);
			
			rebuildViewPorts(v->sheet(), true);
			v->selectMElement(clef);
			v->repaint();
			break;
			
		case CAMusElement::KeySignature: {
			if ( (!context) ||
			    (context->context()->contextType() != CAContext::Staff) )
				return;
			
			staff = (CAStaff*)context->context();
			CAKeySignature *keySig = new CAKeySignature(CAKeySignature::Diatonic, 
			                                            mpoKeySigPSP->getKeySignature()-7,
			                                            CAKeySignature::Major, staff,
			                                            (right?right->timeStart():staff->lastTimeEnd()));
			staff->insertSignBefore(keySig, right);
			
			rebuildViewPorts(v->sheet(), true);
			v->selectMElement(keySig);
			v->repaint();
			break;
		}
		case CAMusElement::Note:
			if ( (!context) ||
			     (context->context()->contextType() != CAContext::Staff) )
				return;

			drawableStaff = (CADrawableStaff*)context;
			staff = drawableStaff->staff();
			note = new CANote(_insertNote,
			                  staff->voiceAt(0),
			                  drawableStaff->calculatePitch(coords.x(), coords.y()),
			                  0,
			                  (right?right->timeStart():staff->lastTimeEnd())
			                 );
			staff->insertNoteBefore(note, right);

			rebuildViewPorts(v->sheet(), true);
			v->selectMElement(note);
			v->repaint();
			break;
	}
}

void CAMainWin::viewPortMouseMoveEvent(QMouseEvent *e, QPoint coords, CAViewPort *v) {
	if ((_currentMode == InsertMode) &&
	    (_insertMusElement == CAMusElement::Note) &&
	    (v->viewPortType()==CAViewPort::ScoreViewPort)
	   ) {
		CAScoreViewPort *c = (CAScoreViewPort*)v;
		CADrawableStaff *s;
		if (c->currentContext()?(c->currentContext()->drawableContextType() == CADrawableContext::DrawableStaff):0)
			s = (CADrawableStaff*)c->currentContext(); 
		else
			return;

		if (_insertMusElement == CAMusElement::Note)
			c->setShadowNoteVisible(true);
		
		//calculate the logical pitch out of absolute world coordinates and the current clef
		int pitch = s->calculatePitch(coords.x(), coords.y());
		
		//write into the main window's status bar the note pitch name
		statusBar()->showMessage(CANote::generateNoteName(pitch));
	}
}

void CAMainWin::viewPortWheelEvent(QWheelEvent *e, QPoint coords, CAViewPort *c) {
	_activeViewPort = c;
	CAScoreViewPort *v = (CAScoreViewPort*) c;

	int val;
	switch (e->modifiers()) {
		case Qt::NoModifier:			//scroll horizontally
			v->setWorldX( v->worldX() - (int)((0.5*e->delta()) / v->zoom()), _animatedScroll );
			break;
		case Qt::AltModifier:			//scroll horizontally, fast
			v->setWorldX( v->worldX() - (int)(e->delta() / v->zoom()), _animatedScroll );
			break;
		case Qt::ShiftModifier:			//scroll vertically
			v->setWorldY( v->worldY() - (int)((0.5*e->delta()) / v->zoom()), _animatedScroll );
			break;
		case 0x0A000000://SHIFT+ALT		//scroll vertically, fast
			v->setWorldY( v->worldY() - (int)(e->delta() / v->zoom()), _animatedScroll );
			break;
		case Qt::ControlModifier:		//zoom
			if (e->delta() > 0)
				v->setZoom( v->zoom()*1.1, coords.x(), coords.y(), _animatedScroll );
			else
				v->setZoom( v->zoom()/1.1, coords.x(), coords.y(), _animatedScroll );
			
			break;
	}

	v->repaint();
}

void CAMainWin::viewPortKeyPressEvent(QKeyEvent *e, CAViewPort *v) {
	_activeViewPort = v;
	
	if (_activeViewPort->viewPortType() == CAViewPort::ScoreViewPort) {
		switch (e->key()) {
			//Cursor keys
			case Qt::Key_Right:
				//select next music element
				((CAScoreViewPort*)_activeViewPort)->selectNextMusElement();
				_activeViewPort->repaint();
				
				break;
			case Qt::Key_Left:
				//select previous music element
				((CAScoreViewPort*)_activeViewPort)->selectPrevMusElement();
				_activeViewPort->repaint();
				
				break;
			case Qt::Key_B: {
				//place a barline
				CADrawableContext *drawableContext;
				drawableContext = ((CAScoreViewPort*)v)->currentContext();
				
				if ( (!drawableContext) || (drawableContext->context()->contextType() != CAContext::Staff) )
					return;
			
				CAStaff *staff = (CAStaff*)drawableContext->context();
				CAMusElement *right = 0;
				if (!((CAScoreViewPort*)v)->selection()->isEmpty())
					right = ((CAScoreViewPort*)v)->selection()->back()->musElement();
					
				CABarline *bar = new CABarline(
					CABarline::Single,
					staff,
					(right?right->timeStart():staff->lastTimeEnd())
				);
				staff->insertSignAfter(bar, right);
				
				rebuildViewPorts(((CAScoreViewPort*)v)->sheet(), true);
				((CAScoreViewPort*)v)->selectMElement(bar);
				v->repaint();
				break;
			}
			case Qt::Key_Up:
				if (_currentMode == SelectMode) {	//select the upper music element
					((CAScoreViewPort*)_activeViewPort)->selectUpMusElement();
					_activeViewPort->repaint();
				} else if (_currentMode == InsertMode) {
					if (!((CAScoreViewPort*)_activeViewPort)->selection()->isEmpty()) {
						CADrawableMusElement *elt =
						((CAScoreViewPort*)_activeViewPort)->selection()->back();
						
						//pitch note for one step higher
						if (elt->drawableMusElementType() == CADrawableMusElement::DrawableNote) {
							CANote *note = (CANote*)elt->musElement();
							note->setPitch(note->pitch()+1);
							rebuildViewPorts(note->voice()->staff()->sheet());
						}
					}
				}
				
				break;
			case Qt::Key_Down:
				if (_currentMode == SelectMode) {	//select the upper music element
					((CAScoreViewPort*)_activeViewPort)->selectUpMusElement();
					_activeViewPort->repaint();
				} else if (_currentMode == InsertMode) {
					if (!((CAScoreViewPort*)_activeViewPort)->selection()->isEmpty()) {
						CADrawableMusElement *elt =
						((CAScoreViewPort*)_activeViewPort)->selection()->back();
						
						//pitch note for one step higher
						if (elt->drawableMusElementType() == CADrawableMusElement::DrawableNote) {
							CANote *note = (CANote*)elt->musElement();
							note->setPitch(note->pitch()-1);
							rebuildViewPorts(note->voice()->staff()->sheet());
						}
					}
				}
				
				break;
			
			//Mode keys
			case Qt::Key_Escape:
				if ((currentMode()==SelectMode) && (_activeViewPort) && (_activeViewPort->viewPortType() == CAViewPort::ScoreViewPort)) {
					((CAScoreViewPort*)_activeViewPort)->selectMElement(0);
					((CAScoreViewPort*)_activeViewPort)->selectContext(0);
				}
				setMode(SelectMode);
				if (mpoKeySigPSP)
					mpoKeySigPSP->hide();
				break;
			case Qt::Key_I:
				_insertMusElement = CAMusElement::Note;
				setMode(InsertMode);
				break;
		}
	}
}

void CAMainWin::keyPressEvent(QKeyEvent *e) {
}

void CAMainWin::initMidi() {
	_midiOut = new CARtMidiDevice();
}

void CAMainWin::playbackFinished() {
	_playback->disconnect();
	//delete _playback;	//TODO: crashes on application close, if deleted! Is this ok? -Matevz
	moMainWin.actionPlay->setChecked(false);
	
	_repaintTimer->stop();
	_repaintTimer->disconnect();	//TODO: crashes, if disconnected sometimes. -Matevz
	delete _repaintTimer;			//TODO: crashes, if deleted. -Matevz
	_midiOut->closePort();
	
	setMode(_currentMode);
}

void CAMainWin::on_actionPlay_toggled(bool checked) {
	if (checked && (_activeViewPort->viewPortType() == CAViewPort::ScoreViewPort)) {
		_midiOut->openPort();
		_repaintTimer = new QTimer();
		_repaintTimer->setInterval(100);
		_repaintTimer->start();
		//connect(_repaintTimer, SIGNAL(timeout()), this, SLOT(on_repaintTimer_timeout()));
		connect(_repaintTimer, SIGNAL(timeout()), _activeViewPort, SLOT(repaint()));
		_playbackViewPort = _activeViewPort;
		
		_playback = new CAPlayback((CAScoreViewPort*)_activeViewPort, _midiOut);
		connect(_playback, SIGNAL(finished()), this, SLOT(playbackFinished()));
		_playback->start();
	} else {
		_playback->stop();
	}
}

/*void CAMainWin::on_repaintTimer_timeout() {
	if (_lockScrollPlayback && (_activeViewPort->viewPortType() == CAViewPort::ScoreViewPort))
		((CAScoreViewPort*)_activeViewPort)->zoomToSelection(_animatedScroll);
	_activeViewPort->repaint();
}*/

void CAMainWin::on_actionAnimated_scroll_toggled(bool val) {
	_animatedScroll = val;
}

void CAMainWin::on_actionLock_scroll_playback_toggled(bool val) {
	_lockScrollPlayback = val;
}

void CAMainWin::on_actionZoom_to_selection_activated() {
	if (_activeViewPort->viewPortType() == CAViewPort::ScoreViewPort)
		((CAScoreViewPort*)_activeViewPort)->zoomToSelection(_animatedScroll);
}

void CAMainWin::on_actionZoom_to_fit_activated() {
	if (_activeViewPort->viewPortType() == CAViewPort::ScoreViewPort)
		((CAScoreViewPort*)_activeViewPort)->zoomToFit();
}

void CAMainWin::on_actionZoom_to_width_activated() {
	if (_activeViewPort->viewPortType() == CAViewPort::ScoreViewPort)
		((CAScoreViewPort*)_activeViewPort)->zoomToWidth();
}

void CAMainWin::on_actionZoom_to_height_activated() {
	if (_activeViewPort->viewPortType() == CAViewPort::ScoreViewPort)
		((CAScoreViewPort*)_activeViewPort)->zoomToHeight();
}

void CAMainWin::on_actionOpen_activated() {
	QString s = QFileDialog::getOpenFileName(
	                this,
	                "Choose a file to open",
	                "",
	                "Canorus document (*.xml)");

	if (s.isEmpty())
		return;

	QFile file(s);
	if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
		_fileName = s;
		_document.clear();
		clearUI();
		
		QXmlInputSource input(&file);
		CACanorusML::openDocument(&input, &_document, this);
		file.close();
		rebuildViewPorts();
		moMainWin.tabWidget->setCurrentIndex(0);
	}               
}

void CAMainWin::on_actionSave_activated() {
	QString s;
	if (_fileName.isEmpty()) { 
		s = QFileDialog::getSaveFileName(
		                this,
		                "Choose a file to save",
		                "",
		                "Canorus document (*.xml)");
	}

	if (s.isEmpty())
		return;

	//append the extension, if the last 4 characters don't already contain the dot
	int i;
	for (i=0; (i<4) && ((s.length()-i-1) > 0); i++) if (s[s.length()-i-1] == '.') break;
	if (i==4) s.append(".xml");
		
	QFile file(s);
	if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
		_fileName = s;
		QTextStream out(&file);
		CACanorusML::saveDocument(out, &_document);
		file.close();
	}               
}

void CAMainWin::on_actionSave_as_activated() {
	QString s = QFileDialog::getSaveFileName(
	                this,
	                "Choose a file to save",
	                "",
	                "Canorus document (*.xml)");
	
	if (s.isEmpty())
		return;
	
	//append the extension, if the last 4 characters don't already contain the dot
	int i;
	for (i=0; (i<4) && ((s.length()-i-1) > 0); i++) if (s[s.length()-i-1] == '.') break;
	if (i==4) s.append(".xml");
	
	QFile file(s);
	if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
		_fileName = s;
		QTextStream out(&file);
		CACanorusML::saveDocument(out, &_document);
		file.close();
	}
}

void CAMainWin::sl_mpoVoiceNum_valChanged(int iVoice)
{
	printf("New voice number: %d\n",iVoice);
	fflush( stdout );
}

void CAMainWin::on_actionNoteSelect_toggled(bool bOn)
{
	// Read currently selected entry from tool button menu
	enum CANote::CANoteLength eElem = (CANote::CANoteLength)
	  mpoMEToolBar->toolElemValue( mpoNoteMenu->objectName() ).toInt();
	_insertMusElement = CAMusElement::Note;
	// New note length type
	_insertNote       = eElem;
	printf("Note Input switched: On %d Note %d\n", bOn, eElem);
	fflush( stdout );
	if( bOn )
		setMode(InsertMode);
}

void CAMainWin::on_actionClefSelect_toggled(bool bOn)
{
	// Read currently selected entry from tool button menu
	enum CAClef::CAClefType eElem = (CAClef::CAClefType)
	  mpoMEToolBar->toolElemValue( mpoClefMenu->objectName() ).toInt();
	_insertMusElement = CAMusElement::Clef;
	// New note length type
	_insertClef       = eElem;
	printf("Note Clef switched: On %d Clef %d\n", bOn, eElem);
	fflush( stdout );
	if( bOn )
		on_action_Clef_activated();
}
	
void CAMainWin::on_action_Key_signature_activated() {
	if (!mpoKeySigPSP) {
		mpoKeySigPSP = new CAKeySigPSP("Edit key signature", this);
		addDockWidget( Qt::LeftDockWidgetArea, mpoKeySigPSP );
	}
	
	mpoKeySigPSP->setWindowIcon(QIcon(QString::fromUtf8(":/menu/images/clogosm.png")));
	mpoKeySigPSP->setFocusPolicy(Qt::ClickFocus);
	mpoKeySigPSP->setFocus();
	mpoKeySigPSP->show();
	
	moMainWin.actionSplit_horizontally->setEnabled(false);
	moMainWin.actionSplit_vertically->setEnabled(false);
	moMainWin.actionUnsplit->setEnabled(true);
	
	setMode(InsertMode);
	_insertMusElement = CAMusElement::KeySignature;
	
}

void CAMainWin::sourceViewPortCommit(QString docString) {
	_document.clear();
	clearUI();
	
	QXmlInputSource input;
	input.setData(docString);
	CACanorusML::openDocument(&input, &_document, this);
	
	on_actionSource_view_perspective_activated();
	rebuildViewPorts();
}

void CAMainWin::on_actionAbout_Qt_activated()
{
	QMessageBox::aboutQt( this, "About Qt" );
}

void CAMainWin::on_actionAbout_Canorus_activated()
{
	QMessageBox::about ( this, "About Canorus",
	"Canorus - The next generation music score editor\n\n\
Version 0.0.2\n\
(C) 2006 Canorus Development team. All rights reserved.\n\
See the file AUTHORS for the list of Canorus developers\n\n\
This program is licensed under the GNU General Public License (GPL).\n\
See the file 'LICENSE.GPL' for details.\n\n\
Homepage: http://canorus.berlios.de" );
}
