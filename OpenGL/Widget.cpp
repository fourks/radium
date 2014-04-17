#include <stdio.h>

#include <vlVG/VectorGraphics.hpp>
#include <vlGraphics/Rendering.hpp>
#include <vlQt4/Qt4ThreadedWidget.hpp>
#include <vlGraphics/SceneManagerActorTree.hpp>
#include <vlVG/SceneManagerVectorGraphics.hpp>

#include <QWidget>
#include <QGLWidget>
#include <QMutex>
#include <QTextEdit>
#include <QMessageBox>
#include <QApplication>
#include <QAbstractButton>

#include "../common/nsmtracker.h"
#include "../common/playerclass.h"
#include "../common/list_proc.h"
#include "../common/realline_calc_proc.h"
#include "../common/time_proc.h"
#include "../common/settings_proc.h"

#define GE_DRAW_VL
#include "GfxElements.h"
#include "Timing.hpp"


#include "Widget_proc.h"



#include <sys/types.h>
#include <unistd.h>
#include <errno.h>

static int set_pthread_priority(pthread_t pthread,int policy,int priority,const char *message,const char *name){
  struct sched_param par={0};
  par.sched_priority=priority;

  //if((sched_setscheduler(pid,policy,&par)!=0)){
  if ((pthread_setschedparam(pthread, policy, &par)) != 0) {
    fprintf(stderr,message,policy==SCHED_FIFO?"SCHED_FIFO":policy==SCHED_RR?"SCHED_RR":policy==SCHED_OTHER?"SCHED_OTHER":"SCHED_UNKNOWN",priority,getpid(),name,strerror(errno));
    //abort();
    return 0;
  }
  return 1;
}

static void set_realtime(int type, int priority){
  //bound_thread_to_cpu(0);
  set_pthread_priority(pthread_self(),type,priority,"Unable to set %s/%d for %d (\"%s\"). (%s)\n", "a gc thread");
}


static QMutex mutex;

void GL_lock(void){
  mutex.lock();
}

void GL_unlock(void){
  mutex.unlock();
}

static int GE_num_reallines(void){
  return root->song->tracker_windows->wblock->num_reallines;
}

static int GE_curr_realline(void){
  return root->song->tracker_windows->wblock->curr_realline;
}

static float GE_scroll_pos(double realline){
  double extra = root->song->tracker_windows->wblock->top_realline - GE_curr_realline();
  return
    (   (realline+extra) * root->song->tracker_windows->fontheight  );
}



extern PlayerClass *pc;
extern struct Root *root;

extern int scrolls_per_second;
extern int default_scrolls_per_second;

static double get_realline_stime(struct WBlocks *wblock, int realline){  
  return Place2STime(wblock->block, &wblock->reallines[realline]->l.p);
}

TimeEstimator time_estimator;

static void find_current_wblock_and_realline(struct Tracker_Windows *window, struct WBlocks **wblock, double *realline){
  double current_stime = time_estimator.get((double)pc->start_time * 1000.0 / (double)pc->pfreq, pc->block->reltempo) * (double)pc->pfreq / 1000.0;

  double block_stime;

  if (pc->playtype==PLAYBLOCK || pc->playtype==PLAYBLOCK_NONLOOP) {
    *wblock = window->wblock;
    double current_block_sduration = getBlockSTimeLength((*wblock)->block);
    block_stime = fmod(current_stime, current_block_sduration);
  } else {
    abort();
    block_stime = 0.0;
  }

  double prev_line_stime = 0.0;
  int i_realline;
  for(i_realline=1; i_realline<(*wblock)->num_reallines; i_realline++){
    double curr_line_stime = get_realline_stime(*wblock, i_realline);
    if (curr_line_stime >= block_stime) {
      *realline = scale_double(block_stime,
                               prev_line_stime, curr_line_stime,
                               i_realline-1, i_realline);
      return;
    }
    prev_line_stime = curr_line_stime;
  }

  *realline = (*wblock)->num_reallines-0.001; // should not happen that we get here.
}


#if 0
static void UpdateReallineByLines(struct Tracker_Windows *window, struct WBlocks *wblock, double d_till_curr_realline){
  static STime last_time = 0;
  static struct WBlocks *last_wblock = NULL;

  int till_curr_realline = (int)d_till_curr_realline;

  if(scrolls_per_second==-1)
    scrolls_per_second = SETTINGS_read_int("scrolls_per_second", default_scrolls_per_second);

  bool do_scrolling = false;
  
  if(wblock != last_wblock)
    do_scrolling = true;
  
  else if (last_time > pc->therealtime)
    do_scrolling = true;
  
  else if(till_curr_realline < wblock->curr_realline)
    do_scrolling = true;
  
  else if(till_curr_realline > wblock->curr_realline){
    STime from_time = (STime) ((double)Place2STime(wblock->block, &wblock->reallines[wblock->curr_realline]->l.p) / wblock->block->reltempo);
    STime to_time   = (STime) ((double)Place2STime(wblock->block, &wblock->reallines[till_curr_realline]->l.p) / wblock->block->reltempo);
    
    STime time = to_time - from_time;
    
    STime time_necessary_to_scroll = pc->pfreq / scrolls_per_second;
    
    if(time>=time_necessary_to_scroll)
      do_scrolling = true;
  }
  
  if(do_scrolling==true) {
    ScrollEditorToRealLine(window,wblock,till_curr_realline);
    last_time = pc->therealtime;
    last_wblock = wblock;
  }
}
#endif


static EditorWidget *get_editorwidget(void){
  return (EditorWidget *)root->song->tracker_windows->os_visual.widget;
}

static QMouseEvent translate_qmouseevent(const QMouseEvent *qmouseevent){
  const QPoint p = qmouseevent->pos();

  return QMouseEvent(qmouseevent->type(),
                     QPoint(p.x(), p.y() + root->song->tracker_windows->wblock->t.y1),
                     qmouseevent->globalPos(),
                     qmouseevent->button(),
                     qmouseevent->buttons()
                     );
}

class MyQt4ThreadedWidget : public vlQt4::Qt4ThreadedWidget, public vl::UIEventListener {

public:
  
  vl::ref<vl::Rendering> _rendering;
  vl::ref<vl::Transform> _scroll_transform;
  vl::ref<vl::Transform> _linenumbers_transform;
  vl::ref<vl::Transform> _scrollbar_transform;
  vl::ref<vl::SceneManagerActorTree> _sceneManager;

  vl::ref<vl::VectorGraphics> vg;
  vl::ref<vl::SceneManagerVectorGraphics> vgscene;

  volatile bool training_estimator;
  volatile double override_vblank_value;
  bool has_overridden_vblank_value;

  float last_pos;


  MyQt4ThreadedWidget(vl::OpenGLContextFormat vlFormat, QWidget *parent=0)
    : Qt4ThreadedWidget(vlFormat, parent)
    , training_estimator(true)
    , override_vblank_value(-1.0)
    , has_overridden_vblank_value(false)
    , last_pos(-1.0f)
  {
    setMouseTracking(true);
  }

  ~MyQt4ThreadedWidget(){
    fprintf(stderr,"Exit myqt4threaderwidget\n");
  }

  virtual void init_vl(vl::OpenGLContext *glContext) {

    printf("init_vl\n");

    _rendering = new vl::Rendering;
    _rendering->renderer()->setFramebuffer(glContext->framebuffer() );
    //_rendering->camera()->viewport()->setClearColor( vl::green );

    _scroll_transform = new vl::Transform;
    _rendering->transform()->addChild(_scroll_transform.get());

    _linenumbers_transform = new vl::Transform;
    _rendering->transform()->addChild(_linenumbers_transform.get());

    _scrollbar_transform = new vl::Transform;
    _rendering->transform()->addChild(_scrollbar_transform.get());


    // installs a SceneManagerActorTree as the default scene manager
    
    _sceneManager = new vl::SceneManagerActorTree;
    _rendering->sceneManagers()->push_back(_sceneManager.get());

#if 0
    vl::mat4 mat = vl::mat4::getTranslation(0,0,0);
    mat = vl::mat4::getTranslation(0,0,0) * mat;
    _rendering->camera()->setViewMatrix( mat );
#endif

    glContext->initGLContext();
    glContext->addEventListener(this);

    if(0)set_realtime(SCHED_FIFO,1);
  }

    /** Event generated when the bound OpenGLContext bocomes initialized or when the event listener is bound to an initialized OpenGLContext. */
  virtual void initEvent() {
    printf("initEvent\n");
    
    _rendering->sceneManagers()->clear();
    
    vg = new vl::VectorGraphics;
    
    vgscene = new vl::SceneManagerVectorGraphics;
    vgscene->vectorGraphicObjects()->push_back(vg.get());
    _rendering->sceneManagers()->push_back(vgscene.get());
  }

private:
  bool canDraw(){
    if (vg.get()==NULL)
      return false;
    else
      return true;
  }
public:

  virtual void mouseReleaseEvent( QMouseEvent *qmouseevent){
    QMouseEvent event = translate_qmouseevent(qmouseevent);
    get_editorwidget()->mouseReleaseEvent(&event);
  }

  virtual void mousePressEvent( QMouseEvent *qmouseevent){
    QMouseEvent event = translate_qmouseevent(qmouseevent);
    get_editorwidget()->mousePressEvent(&event);
  }

  virtual void mouseMoveEvent( QMouseEvent *qmouseevent){
    QMouseEvent event = translate_qmouseevent(qmouseevent);
    get_editorwidget()->mouseMoveEvent(&event);
  }

  /** Event generated right before the bound OpenGLContext is destroyed. */
  virtual void destroyEvent() {
    fprintf(stderr,"destroyEvent\n");
  }

private:
  float find_pos(){
    if(pc->isplaying) {
      NInt            curr_block           = root->curr_block;
      struct WBlocks *wblock               = (struct WBlocks *)ListFindElement1(&root->song->tracker_windows->wblocks->l,curr_block);
      double          d_till_curr_realline = wblock->till_curr_realline;
      
      find_current_wblock_and_realline(root->song->tracker_windows, &wblock, &d_till_curr_realline);
      
      return GE_scroll_pos(d_till_curr_realline);
      
    } else

      return GE_scroll_pos(GE_curr_realline());
  }
public:

  /** Event generated when the bound OpenGLContext does not have any other message to process 
      and OpenGLContext::continuousUpdate() is set to \p true or somebody calls OpenGLContext::update(). */
  virtual void updateEvent() {
    //printf("updateEvent\n");

    if (has_overridden_vblank_value==false && override_vblank_value > 0.0) {

      time_estimator.set_vblank(override_vblank_value);
      training_estimator = false;
      has_overridden_vblank_value = true;

    } else {

      if(training_estimator)
        training_estimator = time_estimator.train();
        
      if(training_estimator) {
        if ( openglContext()->hasDoubleBuffer() )
          openglContext()->swapBuffers();
        return;
      }

    }


    if(canDraw()) {

      bool is_redrawn = false;

      if(GE_new_read_contexts()==true) {
        vg->clear();
        GE_draw_vl(_rendering->camera()->viewport(), vg, _scroll_transform, _linenumbers_transform, _scrollbar_transform);
        is_redrawn = true;
      }

      float pos = find_pos();

      if (is_redrawn || pos!=last_pos) {
    
        // scroll
        {
          vl::mat4 mat = vl::mat4::getRotation(0.0f, 0, 0, 1);
          mat.translate(0,pos,0);
          _scroll_transform->setLocalAndWorldMatrix(mat);
        }

        // linenumbers
        {
          vl::mat4 mat = vl::mat4::getRotation(0.0f, 0, 0, 1);
          mat.translate(0,pos,0);
          _linenumbers_transform->setLocalAndWorldMatrix(mat);
        }
      
        // scrollbar
        {
          vl::mat4 mat = vl::mat4::getRotation(0.0f, 0, 0, 1);
          mat.translate(0,
                        scale(pos,
                              0,512*20 + 1200,
                              _rendering->camera()->viewport()->height(),100
                              ),
                        0);

          _scrollbar_transform->setLocalAndWorldMatrix(mat);
        }

        _rendering->render();

        last_pos = pos;
      }


      GL_lock();
      {
        // show rendering
        if ( openglContext()->hasDoubleBuffer() )
          openglContext()->swapBuffers();
      }
      GL_unlock();
    }
  }

  void set_vblank(double value){
    override_vblank_value = value;
  }
    
  /** Event generated whenever setEnabled() is called. */
  virtual void enableEvent(bool enabled){
    printf("enableEvent %d\n",(int)enabled);
  }

  /** Event generated whenever a listener is bound to an OpenGLContext context. */
  virtual void addedListenerEvent(vl::OpenGLContext*) {
    printf("addedListenerEvent\n");
  }

  /** Event generated whenever a listener is unbound from an OpenGLContext context. */
  virtual void removedListenerEvent(vl::OpenGLContext*) {
    printf("removedListenerEvent\n");
  }
  
  /** Event generated when the mouse moves. */
  virtual void mouseMoveEvent(int x, int y) {
    printf("mouseMove %d %d\n",x,y);
    //_rendering->sceneManagers()->clear();
    //create_block();
    //initEvent();
  }
  
  /** Event generated when one of the mouse buttons is released. */
  virtual void mouseUpEvent(vl::EMouseButton button, int x, int y) {
    printf("mouseMove %d %d\n",x,y);
    //_rendering->sceneManagers()->clear();
    //create_block(_rendering->camera()->viewport()->width(), _rendering->camera()->viewport()->height());
    //initEvent();
  }
  
  /** Event generated when one of the mouse buttons is pressed. */
  virtual void mouseDownEvent(vl::EMouseButton button, int x, int y) {
    printf("mouseMove %d %d\n",x,y);
    //_rendering->sceneManagers()->clear();
    //create_block();
    //initEvent(); 
  }
  
  /** Event generated when the mouse wheel rotated. */
  virtual void mouseWheelEvent(int n) {
  }
  
  /** Event generated when a key is pressed. */
  virtual void keyPressEvent(unsigned short unicode_ch, vl::EKey key) {
    printf("key pressed\n");

    //_rendering->sceneManagers()->clear();
    //create_block();
    //initEvent();
  }
  
  /** Event generated when a key is released. */
  virtual void keyReleaseEvent(unsigned short unicode_ch, vl::EKey key) {
  }
  
  /** Event generated when the bound OpenGLContext is resized. */
  virtual void resizeEvent(int w, int h) {
    _rendering->sceneManagers()->clear();

    _rendering->camera()->viewport()->setWidth(w);
    _rendering->camera()->viewport()->setHeight(h);
    _rendering->camera()->setProjectionOrtho(-0.5f);

    GE_set_height(h);
    //create_block(_rendering->camera()->viewport()->width(), _rendering->camera()->viewport()->height());

    initEvent();

    updateEvent();
  }
  
  /** Event generated when one or more files are dropped on the bound OpenGLContext's area. */
  virtual void fileDroppedEvent(const std::vector<vl::String>& files) {
  }
  
  /** Event generated when the bound OpenGLContext is shown or hidden. */
  virtual void visibilityEvent(bool visible) {
    printf("visibilityEvent %d\n",(int)visible);
  }
};


static vl::ref<MyQt4ThreadedWidget> widget;

static bool do_estimate_questionmark(){
  return SETTINGS_read_bool("use_estimated_vblank", true);
}

static void store_do_estimate(bool value){
  return SETTINGS_write_bool("use_estimated_vblank", value);
}

static bool have_earlier_estimated_value(){
  return SETTINGS_read_double("vblank", -1.0) > 0.0;
}

static double get_earlier_estimated(){
  return 1000.0 / SETTINGS_read_double("vblank", 60.0);
}

static void store_estimated_value(double period){
  printf("***************** Storing %f\n",1000.0 / period);
  SETTINGS_write_double("vblank", 1000.0 / period);
}

static void show_message_box(QMessageBox *box){
  box->setText("Please wait, estimating vblank refresh rate. This takes 3 - 10 seconds");
  box->setInformativeText("!!! Don't move the mouse or press any key !!!");

  if(have_earlier_estimated_value()){

    QString message = QString("Don't estimate again. Use last estimated value instead (")+QString::number(1000.0/get_earlier_estimated())+" Hz).";
    QAbstractButton *msgBox_useStoredValue = (QAbstractButton*)box->addButton(message,QMessageBox::ApplyRole);
    if(0)printf((char*)msgBox_useStoredValue);
    box->show();

  } else {

    box->show();
    box->button(QMessageBox::Ok)->hide();

  }

  qApp->processEvents();
}

static void setup_widget(void){
  vl::VisualizationLibrary::init();

  vl::OpenGLContextFormat vlFormat;
  vlFormat.setDoubleBuffer(true);
  vlFormat.setRGBABits( 8,8,8,8 );
  vlFormat.setDepthBufferBits(24);
  vlFormat.setFullscreen(false);
  vlFormat.setMultisampleSamples(4); // multisampling 32 seems to make text more blurry. 16 sometimes makes program crawl in full screen (not 32 though).
  //vlFormat.setMultisampleSamples(8); // multisampling 32 seems to make text more blurry. 16 sometimes makes program crawl in full screen (not 32 though).
  vlFormat.setMultisample(true);
  //vlFormat.setMultisample(false);
  vlFormat.setVSync(true);
  
  widget = new MyQt4ThreadedWidget(vlFormat, NULL);
  widget->resize(1000,1000);
  widget->show();

  widget->incReference();  // dont want auto-desctruction at program exit.
}

QWidget *GL_create_widget(void){

  if (do_estimate_questionmark() == false) {

    setup_widget();
    widget->set_vblank(get_earlier_estimated());

  } else {

    QMessageBox box;

    setup_widget();
    show_message_box(&box);

    while(widget->training_estimator==true) {
      if(box.clickedButton()!=NULL){
        widget->set_vblank(get_earlier_estimated());
        store_do_estimate(false);
        break;
      }
      qApp->processEvents();
    }

    if (box.clickedButton()==NULL)
      store_estimated_value(time_estimator.get_vblank());

    box.close();
  }

  return widget.get();
}


void GL_stop_widget(QWidget *widget){
  MyQt4ThreadedWidget *mywidget = static_cast<MyQt4ThreadedWidget*>(widget);

  mywidget->stop();
  //delete mywidget;
}