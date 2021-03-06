/* Copyright 2012 Kjetil S. Matheussen

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA. */

// Based on: (although probably not much left of anymore)


/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the demonstration applications of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <algorithm>
#include <set>

#include <QMainWindow>

#include "../common/nsmtracker.h"
#include "../common/hashmap_proc.h"
#include "../common/undo.h"
#include "../common/instruments_proc.h"
#include "../common/OS_Player_proc.h"

#include "undo_mixer_proc.h"

#include "QM_MixerWidget.h"
#include "QM_chip.h"
#include "undo_chip_position_proc.h"

#include "../audio/audio_instrument_proc.h"
#include "../audio/SoundPlugin_proc.h"
#include "../audio/SoundProducer_proc.h"
#include "../audio/Mixer_proc.h"
#include "../audio/undo_audio_effect_proc.h"
#include "../audio/CpuUsage.hpp"
#include "../Qt/EditorWidget.h"
#include "../Qt/Qt_instruments_proc.h"
//#include "../Qt/Qt_MyQCheckBox.h"
#include "../Qt/Qt_mix_colors.h"
#include "../Qt/Qt_colors_proc.h"

#include "../common/patch_proc.h"

extern EditorWidget *g_editor;

//
// eport/econnection means "event port"/"event connection".
//


static int get_text_width(const QFont &font, const QString &text){
  const QFontMetrics fn = QFontMetrics(font);
  return fn.width(text);
}

#if 0
static const int left_border   = 2;
static const int right_border  = 2;
static const int top_border    = 2;
//static const int bottom_border = 2;
#endif

//static const int chip_width = 120;
//static const int slider_height = 15;

static const int name_height = 14;
static const int button_width = 15;//name_height/2;
//static const int button_height = 13;//name_height/2;



// coordinates

static int chip_box_x1 = port_width;
static int chip_box_x2 = chip_width - port_width;
static int chip_box_y1 = port_height;
static int chip_box_y2 = chip_height-port_height;

static void get_coordinates(int &x1, int &y1, int &x2, int &y2){
  x1=chip_box_x1;
  y1=chip_box_y1;
  x2=chip_box_x2;
  y2=chip_box_y2;
}

static void get_name_coordinates(int &x1, int &y1, int &x2, int &y2){
  get_coordinates(x1,y1,x2,y2);
  //get_slider2_coordinates(x1,y1,x2,y2);

  x1 = x1 + name_height;
  y1 = y2 - name_height;

  //  y1+=slider_height;
  //  y2+=name_height;
}

static void get_note_indicator_coordinates(int &x1, int &y1, int &x2, int &y2){
  get_name_coordinates(x1,y1,x2,y2);

  x2 = x1 - 3; 
  x1 = chip_box_x1 + 3;
  y1 += 3;
  y2 -= 3;
}

static int get_note_indicator_x2(void){
  int x1,x2,y1,y2;
  get_note_indicator_coordinates(x1,y1,x2,y2);
  return x2;
}

void CHIP_get_name_coordinates(int &x1, int &y1, int &x2, int &y2){
  get_name_coordinates(x1,y1,x2,y2);
}

void CHIP_get_note_indicator_coordinates(int &x1, int &y1, int &x2, int &y2){
  get_note_indicator_coordinates(x1,y1,x2,y2);
}

static void get_slider1_coordinates(int &x1, int &y1, int &x2, int &y2){
  get_name_coordinates(x1,y1,x2,y2);

  x1 = chip_box_x1;// + left_border;
  x2 = chip_box_x2;// - right_border;
  y2 = y1;
  y1 = chip_box_y1; // + top_border;
}


static void get_slider2_coordinates(int &x1, int &y1, int &x2, int &y2){
  get_slider1_coordinates(x1,y1,x2,y2);
  //y1+=slider_height;
  //y2+=slider_height;
}


static void get_volume_onoff_coordinates(int &x1, int &y1, int &x2, int &y2){
  get_coordinates(x1,y1,x2,y2);

  x1 = x2 - button_width*3;
  x2 = x2 - button_width*2;

  y2 = y1 + button_width - 1;
}

static void get_solo_onoff_coordinates(int &x1, int &y1, int &x2, int &y2){
  get_coordinates(x1,y1,x2,y2);

  x1 = x2 - button_width*2;
  x2 = x2 - button_width;

  y2 = y1 + button_width - 1;
}

static void get_effects_onoff_coordinates(int &x1, int &y1, int &x2, int &y2){
  get_coordinates(x1,y1,x2,y2);

  x1 = x2 - button_width;

  y2 = y1 + button_width - 1;
}



// audio port coordinates (port coordinates)

int CHIP_get_input_port_x(Chip *chip){
  return chip->x() + port_width/2 + 2;
}

static int get_input_port_x1(Chip *chip){
  return CHIP_get_input_port_x(chip)-port_width/2;
}

static int get_input_port_x2(Chip *chip){
  return CHIP_get_input_port_x(chip) + port_width/2;
}

int CHIP_get_output_port_x(Chip *chip){
  QRectF rect = chip->boundingRect();
  return chip->x()+rect.width() - port_width/2 - 2;
}

static int get_output_port_x1(Chip *chip){
  return CHIP_get_output_port_x(chip)-port_width/2;
}

static int get_output_port_x2(Chip *chip){
  return CHIP_get_output_port_x(chip) + port_width/2;
}

int CHIP_get_port_y(Chip *chip){
  QRectF rect = chip->boundingRect();
  //printf("y: %d, y2: %d\n",(int)chip->y(),(int)(chip->y()+rect.height()));
  return chip->y() + rect.height()/2;
}

static int get_port_y1(Chip *chip){
  //QRectF rect = chip->boundingRect();
  return chip->y();
}

static int get_port_y2(Chip *chip){
  QRectF rect = chip->boundingRect();
  return chip->y() + rect.height();
}

bool CHIP_is_at_input_port(Chip *chip, int x, int y){
  return (x >= get_input_port_x1(chip))
    && (x < get_input_port_x2(chip))
    && (y >= get_port_y1(chip))
    && (y < get_port_y2(chip));
}

bool CHIP_is_at_output_port(Chip *chip, int x, int y){
  return (x >= get_output_port_x1(chip))
    && (x < get_output_port_x2(chip))
    && (y >= get_port_y1(chip))
    && (y < get_port_y2(chip));
}


// control port coordinates (eport coordinates)

int CHIP_get_eport_x(Chip *chip){
  int x1,y1,x2,y2;
  get_note_indicator_coordinates(x1,y1,x2,y2);
  return chip->x() + (x1+x2)/2;
  //return chip->x() + chip_width/2;
}

static int get_eport_x1(Chip *chip){
  return chip->x();
  //return CHIP_get_eport_x(chip)-port_width/2;
}

static int get_eport_x2(Chip *chip){ 
  int x1,y1,x2,y2;
  get_effects_onoff_coordinates(x1,y1,x2,y2);
  return chip->x() + x1;
  //return CHIP_get_eport_x(chip) + port_width/2;
}

int CHIP_get_output_eport_y(Chip *chip){
  int x1,y1,x2,y2;
  get_note_indicator_coordinates(x1,y1,x2,y2);
  return chip->y() + y2;
}

int CHIP_get_input_eport_y(Chip *chip){
  int x1,y1,x2,y2;
  get_note_indicator_coordinates(x1,y1,x2,y2);
  return chip->y()+y1;
}

static int get_output_eport_y1(Chip *chip){
  int x1,y1,x2,y2;
  get_name_coordinates(x1,y1,x2,y2);

  return chip->y() + y1;
}

static int get_output_eport_y2(Chip *chip){
  return chip->y() + grid_height;
  //return CHIP_get_output_eport_y(chip)+port_height/2;
}

static int get_input_eport_y1(Chip *chip){
  //return get_output_eport_y1(chip);
  return chip->y();
}

static int get_input_eport_y2(Chip *chip){
  //return get_output_eport_y2(chip);
  return chip->y() + chip_box_y1;
}

bool CHIP_is_at_input_eport(Chip *chip, int x, int y){
  return (x >= get_eport_x1(chip))
    && (x < get_eport_x2(chip))
    && (y >= get_input_eport_y1(chip))
    && (y < get_input_eport_y2(chip));
}

bool CHIP_is_at_output_eport(Chip *chip, int x, int y){
  printf("x/y: %d/%d. x1/y1: %d/%d. x2/y2: %d/%d\n",x,y,
         get_eport_x1(chip),get_output_eport_y1(chip),
         get_eport_x2(chip),get_output_eport_y2(chip));

  return (x >= get_eport_x1(chip))
    && (x < get_eport_x2(chip))
    && (y >= get_output_eport_y1(chip))
    && (y < get_output_eport_y2(chip));
}



// stuff 


static void paint_checkbutton(QPainter *painter, const QString &name, const QColor &text_color, const QColor &background_color, int x1, int y1, int x2, int y2, bool is_on){

  {
    QColor c = get_qcolor(MIXER_BORDER_COLOR_NUM);
    painter->setPen(QPen(c, 0.5));
  
    //painter->drawRect(x1, y1, x2-x1, y2-y1);
    if (name=="M"){
      QLineF line(x1-0.25, y1+0.5, x1-0.25, y2-0.25);
      painter->drawLine(line);
    }
    
    if (name!="B"){
      QLineF line(x2-0.25, y1+0.5, x2-0.25, y2-0.25);
      painter->drawLine(line);
    }
  }

  
  if(!is_on){

    QColor c = background_color;

    /*
    painter->drawLine(x1, y1, x2, y2);
    painter->drawLine(x2, y1, x1, y2);
    */
    
    //#else
    //QColor c(10,12,30,65);
    //painter->setPen(QPen(c, 2));
    c.setAlpha(200);
    QRectF rect(x1+0.25, y1+0.75, x2-x1-1, y2-y1-1.25);
    painter->fillRect(rect, c);
    //#endif

    /*
    c.setAlpha(255);
    painter->drawLine(x1, y1, x2, y2);
    painter->drawLine(x2, y1, x1, y2);
    */
  }

  {
    painter->setPen(QPen(text_color, 2));

    painter->drawText(x1+1, y1+1, x2-x1-1, y2-y1-1, Qt::AlignCenter, name);
  }
}

Chip *find_chip_for_plugin(QGraphicsScene *scene, SoundPlugin *plugin){
  QList<QGraphicsItem *> das_items = scene->items();

  for (int i = 0; i < das_items.size(); ++i) {
    Chip *chip = dynamic_cast<Chip*>(das_items.at(i));
    if(chip!=NULL){
      if(plugin==SP_get_plugin(chip->_sound_producer))
        return chip;
    }
  }
  return NULL;
}

void CHIP_update(SoundPlugin *plugin){
  Chip *chip = find_chip_for_plugin(&g_mixer_widget->scene, plugin);

  int eff1 = plugin->type->num_effects+EFFNUM_INPUT_VOLUME;
  int eff2 = plugin->type->num_effects+EFFNUM_VOLUME;

  int val1 = scale(PLUGIN_get_effect_value(plugin, eff1, VALUE_FROM_STORAGE), 0,1,0,10000);
  int val2 = scale(PLUGIN_get_effect_value(plugin, eff2, VALUE_FROM_STORAGE), 0,1,0,10000);

  SLIDERPAINTER_setValue(chip->_input_slider, val1);
  SLIDERPAINTER_setValue(chip->_output_slider, val2);

#if 0
  {
    char buf[64]={0};
    PLUGIN_get_display_value_string(plugin, eff1, buf, 64);
    SLIDERPAINTER_set_string(chip->_input_slider, QString(buf));
  }

  {
    char buf[64]={0};
    PLUGIN_get_display_value_string(plugin, eff2, buf, 64);
    SLIDERPAINTER_set_string(chip->_output_slider, QString(buf));
  }
#endif

  chip->update();
}

/*
void CHIP_init_because_it_has_new_plugin(SoundPlugin *plugin){
  Chip *chip = find_chip_for_plugin(&g_mixer_widget->scene, plugin);
  chip->init_new_plugin();
}
*/

float CHIP_get_pos_x(const struct Patch *patch){
  Chip *chip = find_chip_for_plugin(&g_mixer_widget->scene, (SoundPlugin*)patch->patchdata);
  if (chip==NULL)
    return 0;
  return chip->x();
}

float CHIP_get_pos_y(const struct Patch *patch){
  Chip *chip = find_chip_for_plugin(&g_mixer_widget->scene, (SoundPlugin*)patch->patchdata);
  if (chip==NULL)
    return 0;
  return chip->y();
}

void CHIP_set_pos(struct Patch *patch,float x,float y){
  Chip *chip = find_chip_for_plugin(&g_mixer_widget->scene, (SoundPlugin*)patch->patchdata);
  if (chip!=NULL)
    chip->setPos(x,y);
}

static void CHIP_kick_left_rec(Chip *chip, std::set<Chip*> &kicks){
  QPointF pos=chip->pos();

#if 0
  // First move chips which are connected. (but only if they are placed to the right of this chip)
  for(unsigned int i=0;i<chip->audio_connections.size();i++){
    Connection *connection = chip->audio_connections.at(i);
    if(connection->from==chip){
      Chip *chip2 = connection->to;
      if(chip2->pos().x() >= pos.x())
        CHIP_kick_right_rec(chip2, kicks);
    }
  }
#endif

  // Then move chip placed to the right
  Chip *chip2 = MW_get_chip_at(pos.x()-grid_width + chip_width/2, pos.y() + chip_height/2,chip);
  if(chip2!=NULL)    
    CHIP_kick_left_rec(chip2, kicks);

  // Move the chip itself
  kicks.insert(chip);
}

void CHIP_kick_left(Chip *chip){
  std::set<Chip*> kicks;

  CHIP_kick_left_rec(chip, kicks);

  for(std::set<Chip*>::iterator chip = kicks.begin() ; chip!=kicks.end() ; chip++){
    QPointF pos=(*chip)->pos();
    
    SoundPlugin *plugin = SP_get_plugin((*chip)->_sound_producer);
    volatile struct Patch *patch = plugin->patch;
    R_ASSERT_RETURN_IF_FALSE(patch!=NULL);
    
    ADD_UNDO(ChipPos_CurrPos((struct Patch*)patch));
    (*chip)->setPos(pos.x()-grid_width, pos.y());
  }
}

static void CHIP_kick_right_rec(Chip *chip, std::set<Chip*> &kicks){
  QPointF pos=chip->pos();

#if 0 // Disabled. Too messy.

  // First move chips which are connected. (but only if they are placed to the right of this chip)
  for(unsigned int i=0;i<chip->audio_connections.size();i++){
    Connection *connection = chip->audio_connections.at(i);
    if(connection->from==chip){
      Chip *chip2 = connection->to;
      if(chip2->pos().x() >= pos.x())
        CHIP_kick_right_rec(chip2, kicks);
    }
  }
#endif

  // Then move chip placed to the right
  Chip *chip2 = MW_get_chip_at(pos.x()+grid_width + chip_width/2, pos.y() + chip_height/2,chip);
  if(chip2!=NULL)    
    CHIP_kick_right_rec(chip2, kicks);

  // Move the chip itself
  kicks.insert(chip);
}

void CHIP_kick_right(Chip *chip){
  std::set<Chip*> kicks;

  CHIP_kick_right_rec(chip, kicks);

  for(std::set<Chip*>::iterator chip = kicks.begin() ; chip!=kicks.end() ; chip++){
    QPointF pos=(*chip)->pos();

    SoundPlugin *plugin = SP_get_plugin((*chip)->_sound_producer);
    volatile struct Patch *patch = plugin->patch;
    R_ASSERT_RETURN_IF_FALSE(patch!=NULL);

    ADD_UNDO(ChipPos_CurrPos((struct Patch*)patch));
    (*chip)->setPos(pos.x()+grid_width, pos.y());
  }
}

static bool connect(QGraphicsScene *scene, Chip *from, int from_portnum, Chip *to, int to_portnum){
  return SP_add_link(to->_sound_producer, to_portnum, 
                     from->_sound_producer, from_portnum);
}

static bool econnect(QGraphicsScene *scene, Chip *from, Chip *to){
  if (SP_add_elink(to->_sound_producer, from->_sound_producer) == false)
    return false;

  SoundPlugin *plugin1 = SP_get_plugin(from->_sound_producer);
  volatile struct Patch *patch1 = plugin1->patch;
  R_ASSERT_RETURN_IF_FALSE2(patch1!=NULL, false);

  SoundPlugin *plugin2 = SP_get_plugin(to->_sound_producer);
  volatile struct Patch *patch2 = plugin2->patch;
  R_ASSERT_RETURN_IF_FALSE2(patch2!=NULL, false);

  if (PATCH_add_event_receiver((struct Patch*)patch1,
                               (struct Patch*)patch2)
      == false
      )
    {
      RError("Impossible situation: PATCH_add_event_receiver()==true and SP_add_elink()==false");
      return false;
    }
  
  return true;
}

static bool chips_are_connected(Chip *from, Chip *to){
  for(AudioConnection *connection : from->audio_connections)
    if(connection->from==from && connection->to==to)
      return true;

  return false;
}

static bool chips_are_econnected(Chip *from, Chip *to){
  for(EventConnection *connection : from->event_connections)
    if(connection->from==from && connection->to==to)
      return true;

  return false;
}

void CHIP_connect_chips(QGraphicsScene *scene, Chip *from, Chip *to){
  if(from->_num_outputs==0 || to->_num_inputs==0)
    return;

  if(chips_are_connected(from,to)==true)
    return;

  bool from_is_mono = from->_num_outputs==1;
  bool to_is_mono   = to->_num_inputs==1;

  if(from_is_mono==true){
    for(int to_portnum=0 ; to_portnum<to->_num_inputs ; to_portnum++){
      if(connect(scene, from, 0, to, to_portnum)==false)
        return; // trying to make recursive connection
    }
  }else if(to_is_mono==true){
    for(int from_portnum=0 ; from_portnum<to->_num_outputs ; from_portnum++){
      if(connect(scene, from, from_portnum, to, 0)==false)
        return; // trying to make recursive connection
    }
  }else{
    for(int portnum=0 ; portnum<std::min(from->_num_outputs,to->_num_inputs) ; portnum++){
      if(connect(scene, from, portnum, to, portnum)==false)
        return; // trying to make recursive connection
    }
  }

  AudioConnection *connection = new AudioConnection(scene);
  connection->from = from;
  connection->to = to;
  
  from->audio_connections.push_back(connection);
  to->audio_connections.push_back(connection);
  
  connection->update_position();
  scene->addItem(connection);
}

void CHIP_connect_chips(QGraphicsScene *scene, SoundPlugin *from, SoundPlugin *to){
  CHIP_connect_chips(scene, find_chip_for_plugin(scene, from), find_chip_for_plugin(scene, to));
}

void CHIP_econnect_chips(QGraphicsScene *scene, Chip *from, Chip *to){
  if(chips_are_econnected(from,to)==true){
    printf("Chips are already econnected\n");
    return;
  }

  if(econnect(scene, from, to)==false)
    return; // trying to make recursive connection

  EventConnection *econnection = new EventConnection(scene);
  econnection->from = from;
  econnection->to = to;
  
  from->event_connections.push_back(econnection);
  to->event_connections.push_back(econnection);
  
  econnection->update_position();
  scene->addItem(econnection);
}

void CHIP_econnect_chips(QGraphicsScene *scene, SoundPlugin *from, SoundPlugin *to){
  CHIP_econnect_chips(scene, find_chip_for_plugin(scene, from), find_chip_for_plugin(scene, to));
}


int CHIP_get_num_in_connections(const Patch *patch){
  Chip *chip = CHIP_get(&g_mixer_widget->scene, patch);
  R_ASSERT_RETURN_IF_FALSE2(chip!=NULL,0);

  int num=0;

  for (AudioConnection *connection : chip->audio_connections)
    if(connection->to==chip)
      num++;

  return num;
}

int CHIP_get_num_out_connections(const Patch *patch){
  Chip *chip = CHIP_get(&g_mixer_widget->scene, patch);
  R_ASSERT_RETURN_IF_FALSE2(chip!=NULL,0);

  int num=0;

  for (AudioConnection *connection : chip->audio_connections)
    if(connection->from==chip)
      num++;

  return num;
}

int CHIP_get_num_in_econnections(const Patch *patch){
  Chip *chip = CHIP_get(&g_mixer_widget->scene, patch);
  R_ASSERT_RETURN_IF_FALSE2(chip!=NULL,0);

  int num=0;

  for (EventConnection *econnection : chip->event_connections)
    if(econnection->to==chip)
      num++;

  return num;
}

int CHIP_get_num_out_econnections(const Patch *patch){
  Chip *chip = CHIP_get(&g_mixer_widget->scene, patch);
  R_ASSERT_RETURN_IF_FALSE2(chip!=NULL,0);

  int num=0;

  for (EventConnection *econnection : chip->event_connections)
    if(econnection->from==chip)
      num++;

  return num;
}

struct Patch* CHIP_get_source(const struct Patch *patch, int connectionnum){
  Chip *chip = CHIP_get(&g_mixer_widget->scene, patch);
  R_ASSERT_RETURN_IF_FALSE2(chip!=NULL,0);

  int num=0;

  for (AudioConnection *connection : chip->audio_connections)
    if(connection->to==chip){
      if (num==connectionnum)
        return CHIP_get_patch(connection->from);
      num++;
    }

  return NULL;  
}
  
struct Patch* CHIP_get_dest(const struct Patch *patch, int connectionnum){
  Chip *chip = CHIP_get(&g_mixer_widget->scene, patch);
  R_ASSERT_RETURN_IF_FALSE2(chip!=NULL,0);

  int num=0;

  for (AudioConnection *connection : chip->audio_connections)
    if(connection->from==chip){
      if (num==connectionnum)
        return CHIP_get_patch(connection->to);
      num++;
    }

  return NULL;  
}
  
struct Patch* CHIP_get_esource(const struct Patch *patch, int connectionnum){
  Chip *chip = CHIP_get(&g_mixer_widget->scene, patch);
  R_ASSERT_RETURN_IF_FALSE2(chip!=NULL,0);

  int num=0;

  for (EventConnection *econnection : chip->event_connections)
    if(econnection->to==chip){
      if (num==connectionnum)
        return CHIP_get_patch(econnection->from);
      num++;
    }

  return NULL;  
}
  
struct Patch* CHIP_get_edest(const struct Patch *patch, int connectionnum){
  Chip *chip = CHIP_get(&g_mixer_widget->scene, patch);
  R_ASSERT_RETURN_IF_FALSE2(chip!=NULL,0);

  int num=0;

  for (EventConnection *econnection : chip->event_connections)
    if(econnection->from==chip){
      if (num==connectionnum)
        return CHIP_get_patch(econnection->to);
      num++;
    }

  return NULL;  
}
  
void CONNECTION_delete_an_audio_connection_where_all_links_have_been_removed(AudioConnection *connection){
  Chip *from = connection->from;
  Chip *to = connection->to;

  from->audio_connections.remove(connection);
  to->audio_connections.remove(connection);

  delete connection;
}


void CONNECTION_delete_an_event_connection_where_all_links_have_been_removed(EventConnection *connection){
  Chip *from = connection->from;
  Chip *to = connection->to;

  from->event_connections.remove(connection);
  to->event_connections.remove(connection);
  
  delete connection;
}


void CONNECTION_delete_audio_connection(AudioConnection *connection){
  Chip *from = connection->from;
  Chip *to = connection->to;

  bool from_is_mono = from->_num_outputs==1;
  bool to_is_mono   = to->_num_inputs==1;

  if(from_is_mono==true){
    for(int to_portnum=0 ; to_portnum<to->_num_inputs ; to_portnum++){
      SP_remove_link(to->_sound_producer, to_portnum,
                     from->_sound_producer, 0);
    }
  }else if(to_is_mono==true){
    for(int from_portnum=0 ; from_portnum<to->_num_outputs ; from_portnum++){
      SP_remove_link(to->_sound_producer, 0,
                     from->_sound_producer, from_portnum);
    }
  }else{
    for(int portnum=0 ; portnum<std::min(from->_num_outputs,to->_num_inputs) ; portnum++){
      SP_remove_link(to->_sound_producer, portnum,
                     from->_sound_producer, portnum);
    }
  }

  CONNECTION_delete_an_audio_connection_where_all_links_have_been_removed(connection);
}

void CONNECTION_delete_event_connection(EventConnection *connection){
  Chip *from = connection->from;
  Chip *to = connection->to;

  SoundPlugin *plugin1 = SP_get_plugin(from->_sound_producer);
  volatile struct Patch *patch1 = plugin1->patch;
  R_ASSERT_RETURN_IF_FALSE2(patch1!=NULL, );
  
  SoundPlugin *plugin2 = SP_get_plugin(to->_sound_producer);
  volatile struct Patch *patch2 = plugin2->patch;
  R_ASSERT_RETURN_IF_FALSE2(patch2!=NULL, );
  
  
  PATCH_remove_event_receiver((struct Patch*)patch1,
                              (struct Patch*)patch2);
  
  SP_remove_elink(to->_sound_producer, from->_sound_producer);

  CONNECTION_delete_an_event_connection_where_all_links_have_been_removed(connection);
}

void CONNECTION_delete_connection(SuperConnection *connection){
  if (connection->is_event_connection) {
    R_ASSERT_RETURN_IF_FALSE(dynamic_cast<EventConnection*>(connection) != NULL);
    CONNECTION_delete_event_connection(dynamic_cast<EventConnection*>(connection));
  } else {
    R_ASSERT_RETURN_IF_FALSE(dynamic_cast<AudioConnection*>(connection) != NULL);
    CONNECTION_delete_audio_connection(dynamic_cast<AudioConnection*>(connection));
  }
}

void AudioConnection::update_position(void){
  int x1 = to->pos().x()+port_width/2;
  int x2 = from->pos().x()+grid_width-port_width/2;
  int y1 = CHIP_get_port_y(to);
  int y2 = CHIP_get_port_y(from);
  
  this->setLine(x1,y1,x2,y2);
}

void EventConnection::update_position(void){
  int x1 = CHIP_get_eport_x(from);
  int x2 = CHIP_get_eport_x(to);
  int y1 = CHIP_get_output_eport_y(from);
  int y2 = CHIP_get_input_eport_y(to);
  
  /*
    if (x2>x1) {
    y2 = ::scale(x2-10,x1,x2,y1,y2);
    x2 -= 10;
    } else if (x2<x1) {
    y2 = ::scale(x2+10,x1,x2,y1,y2);
    x2 += 10;
    }
  */
  
  this->setLine(x1,y1,x2,y2);
}

// 'right_chip' is inserted in the middle of 'left_chip' and all chips 'left_chip' sends to.
void CHIP_connect_left(QGraphicsScene *scene, Chip *left_chip, Chip *right_chip){
  std::vector<AudioConnection*> connections_to_delete;
  std::vector<Chip*> chips_to_connect_to;

  for (AudioConnection *connection : left_chip->audio_connections)
    if(connection->from==left_chip){
      connections_to_delete.push_back(connection);
      chips_to_connect_to.push_back(connection->to);
    }

  for(unsigned int i=0;i<connections_to_delete.size();i++)
    CONNECTION_delete_audio_connection(connections_to_delete.at(i));

  for(unsigned int i=0;i<chips_to_connect_to.size();i++)
    CHIP_connect_chips(scene,right_chip,chips_to_connect_to.at(i));

  CHIP_connect_chips(scene,left_chip,right_chip);
}

// 'left_chip' is inserted in the middle of 'right_chip' and all chips 'right_chip' receives from.
void CHIP_connect_right(QGraphicsScene *scene, Chip *left_chip, Chip *right_chip){
  std::vector<AudioConnection*> connections_to_delete;
  std::vector<Chip*> chips_to_connect_to;

  for (AudioConnection *connection : right_chip->audio_connections)
    if(connection->to==right_chip){
      connections_to_delete.push_back(connection);
      chips_to_connect_to.push_back(connection->from);
    }

  for(unsigned int i=0;i<connections_to_delete.size();i++)
    CONNECTION_delete_audio_connection(connections_to_delete.at(i));

  for(unsigned int i=0;i<chips_to_connect_to.size();i++)
    CHIP_connect_chips(scene,chips_to_connect_to.at(i),left_chip);

  CHIP_connect_chips(scene,left_chip,right_chip);
}

void Chip::init_new_plugin(void){

  struct SoundPlugin *plugin = SP_get_plugin(_sound_producer);

  _num_inputs = plugin->type->num_inputs;
  _num_outputs = plugin->type->num_outputs;

  if (_input_slider!=NULL)
    SLIDERPAINTER_delete(_input_slider);
  if (_output_slider!=NULL)
    SLIDERPAINTER_delete(_output_slider);
    
  {
    int x1,x2,y1,y2;
    get_slider1_coordinates(x1,x2,y1,y2);
    _input_slider = SLIDERPAINTER_create(this,x1,x2,y1,y2);
    SLIDERPAINTER_set_alternative_color(_input_slider, true);
    SLIDERPAINTER_set_num_channels(_input_slider, _num_inputs);
  }
  
  {
    int x1,x2,y1,y2;
    get_slider2_coordinates(x1,x2,y1,y2);
    _output_slider = SLIDERPAINTER_create(this,x1,x2,y1,y2);
    SLIDERPAINTER_set_num_channels(_output_slider, _num_outputs);
  }
  
  if(_num_outputs>0)
    SLIDERPAINTER_set_peak_value_pointers(_output_slider, _num_outputs, plugin->output_volume_peak_values_for_chip, false);
  //ATOMIC_SET(plugin->volume_peak_values_for_chip, SLIDERPAINTER_obtain_peak_value_pointers(_output_slider,_num_outputs));
  else if(_num_inputs>0)
    SLIDERPAINTER_set_peak_value_pointers(_input_slider, _num_inputs, plugin->input_volume_peak_values, false);
  //ATOMIC_SET(plugin->input_volume_peak_values_for_chip, SLIDERPAINTER_obtain_peak_value_pointers(_input_slider,_num_inputs));
  
  CHIP_update(plugin);
}

Chip::Chip(QGraphicsScene *scene, SoundProducer *sound_producer, float x, float y)
  : _scene(scene)
  , _sound_producer(sound_producer)
  , _color("white")
  , _input_slider(NULL)
  , _output_slider(NULL)
  , _last_updated_mute(false)
  , _last_updated_solo(false)
  , _last_updated_bypass(false)
  , _last_updated_recording(false)
  , _last_updated_autosuspending(false)
  , _slider_being_edited(0)
{

   setPos(QPointF(x,y));
   //MW_move_chip_to_slot(this, x, y); // unfortunately, this function very often moves the chip to the right unnecessarily.

   scene->addItem(this);
   
 #if 0
   _line_item = new QGraphicsLineItem(0,0,50,50);
   _line_item->setPen(QPen(Qt::black, 2));
   _line_item->setPos(QPointF(x+50,y+50));
   scene->addItem(_line_item);
 #endif

   setFlags(ItemIsSelectable | ItemIsMovable | ItemSendsGeometryChanges);
#ifdef USE_QT5
   setAcceptHoverEvents(true);
#else
   setAcceptsHoverEvents(true);
#endif
   setZValue(10);
   
   init_new_plugin();
   
   printf("New Chip. Inputs: %d, Ouptuts: %d\n",_num_inputs,_num_outputs);
}

void CHIP_autopos(Chip *chip){
  double x,y;
  MW_set_autopos(&x, &y);
  MW_move_chip_to_slot(chip, x, y);
}

void CHIP_autopos(struct Patch *patch){
  CHIP_autopos(CHIP_get(&g_mixer_widget->scene, patch));
}

void CHIP_create(SoundProducer *sound_producer, float x, float y){
  new Chip(&g_mixer_widget->scene, sound_producer, x, y);
}

Chip::~Chip(){
  while(audio_connections.size()>0){
    fprintf(stderr,"Deleting connection. Connections left: %d\n",(int)audio_connections.size());
    CONNECTION_delete_audio_connection(audio_connections[0]);
  }
  while(event_connections.size()>0){
    fprintf(stderr,"Deleting econnection. EConnections left: %d\n",(int)event_connections.size());
    CONNECTION_delete_event_connection(event_connections[0]);
  }

  SLIDERPAINTER_delete(_input_slider);
  SLIDERPAINTER_delete(_output_slider);
}

void CHIP_delete(Patch *patch){
  Chip *chip = CHIP_get(&g_mixer_widget->scene, patch);
  printf("     Deleting chip %p (%s)\n",chip,CHIP_get_patch(chip)->name);
  delete chip;
}

QRectF Chip::boundingRect() const
{
  return QRectF(0, 0, chip_width, chip_height);
}

QPainterPath Chip::shape() const
{
  QPainterPath path;
  path.addRect(0, 0, chip_width, chip_height);
  return path;
}

inline static void CHECKBOX_paint_arc(QPainter *painter, bool is_checked, bool is_enabled, int width, int height){
    QColor col1; // off
    QColor col2; // on

    QColor c(30,20,20,40);

    if(is_enabled==true){
      col1 = c.light(90);
      col2 = c.light(82);
    }else{
      col1 = c.light(116);
      col2 = c.light(110);
    }

    //col2 = QColor(106, 104, 100, 255);
    //col2 = QColor(0, 107, 156, 255);

    if(is_checked==true){
      painter->setPen(col2);
      painter->setBrush(QBrush(col2,Qt::SolidPattern));

      painter->drawEllipse(1,1,width-2,height-2);

      //painter->setBrush(QBrush());
    }else{
      painter->setPen(col2);
      //painter->setBrush(QBrush(col2));

      painter->drawEllipse(1,1,width-2,height-2);

      //painter->setBrush(QBrush());

    }
}

extern QHash<int, QColor> custom_colors;

void Chip::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
  Q_UNUSED(widget);
  
  SoundPlugin *plugin = SP_get_plugin(_sound_producer);
  volatile struct Patch *patch = plugin->patch;
  R_ASSERT_RETURN_IF_FALSE(patch!=NULL);

  bool is_selected = (option->state & QStyle::State_Selected);
  /*
    bool is_over = (option->state & QStyle::State_MouseOver);
  */

  bool is_current_patch = get_current_instruments_gui_patch()==patch;

  QColor text_color = get_qcolor(MIXER_TEXT_COLOR_NUM);
  //if(is_current_patch==false)
  text_color.setAlpha(160);
  

  QColor border_color = get_qcolor(MIXER_BORDER_COLOR_NUM);
  if(is_current_patch==false)
    border_color.setAlpha(160);

  painter->setPen(QPen(border_color, 2));

  // slider
  {
    int x1,y1,x2,y2;
    get_slider2_coordinates(x1,y1,x2,y2);      

    {
      QColor c = get_qcolor(MIXER_BORDER_COLOR_NUM);
      c.setAlpha(10);
      painter->setBrush(QBrush(c,Qt::SolidPattern));
      painter->fillRect(x1,y1,x2-x1,y2-y1, c);
    }

    // input slider
    if(plugin->type->num_outputs==0 && plugin->type->num_inputs>0){
      painter->translate(x1,y1);
      SLIDERPAINTER_paint(_input_slider, painter);
      painter->translate(-x1,-y1);
    }

    // output slider (not painted if input slider is painted)
    if(plugin->type->num_outputs>0){
      painter->translate(x1,y1);
      SLIDERPAINTER_paint(_output_slider, painter);
      painter->translate(-x1,-y1);
    }
  }


  QFont font = g_editor->main_window->font();
  font.setPointSize(12);
  font.setPixelSize(12);

  painter->setFont(font);

  // Draw text
  {
    int x1,y1,x2,y2;
    get_name_coordinates(x1,y1,x2,y2);

    int colornum = patch->colornum;
    QColor patchcolor(custom_colors[colornum]);

    QColor c =
      (false && is_selected)
      ? mix_colors(QColor(30,25,70,60), patchcolor, 0.45)
      : mix_colors(QColor(30,65,70,35), patchcolor, 0.05);

    if (ATOMIC_GET(patch->is_recording))
      c = mix_colors(c, QColor(255,0,0), 0.1);

    painter->fillRect(x1, y1, x2-x1, y2-y1, c);
    
    x1 += 2;    
    x2 -= 1;
    
    if (ATOMIC_GET(g_show_cpu_usage_in_mixer)){
      
      CpuUsage *cpu_usage = (CpuUsage*)ATOMIC_GET(plugin->cpu_usage);

      if (cpu_usage==NULL)
        ATOMIC_SET(plugin->cpu_usage, new CpuUsage);
      else
        _name_text = cpu_usage->update_and_get_string();
      
    } else  {
      
      _name_text = patch->name;
      
    }
    
    float textlen = get_text_width(painter->font(),_name_text);       
    float width = x2-x1;

    //printf("updating %d\n",(int)::scale(patch->visual_note_intencity, MAX_NOTE_INTENCITY, 0, 150, 100));
    painter->setPen(QPen(text_color, 2));

    if(textlen>=width){
      float s = width/textlen;
      painter->save();
      painter->scale(s,1.0);
      painter->drawText(x1/s, y1, width/s, y2-y1, Qt::AlignLeft, _name_text);
      painter->restore();
    }else{
      painter->drawText(x1, y1, width, y2-y1, Qt::AlignLeft, _name_text);
    }

    // checbuttons.
    {

      font.setPointSize(10);
      font.setPixelSize(10);

      painter->setFont(font);

      int x1,y1,x2,y2;
      get_volume_onoff_coordinates(x1,y1,x2,y2);
      paint_checkbutton(painter, "M", text_color, Qt::green, x1,y1,x2,y2, ATOMIC_GET(plugin->volume_is_on));

      get_solo_onoff_coordinates(x1,y1,x2,y2);
      paint_checkbutton(painter, "S", text_color, Qt::yellow, x1,y1,x2,y2, !ATOMIC_GET(plugin->solo_is_on));

      get_effects_onoff_coordinates(x1,y1,x2,y2);
      paint_checkbutton(painter, "B", text_color, get_qcolor(ZOOMLINE_TEXT_COLOR_NUM1), x1,y1,x2,y2, ATOMIC_GET(plugin->effects_are_on));
    }
  }


  // Draw note indicator
  {
    int x1,y1,x2,y2;
    get_note_indicator_coordinates(x1,y1,x2,y2);

    int intencity = ATOMIC_GET(patch->visual_note_intencity);
    if(intencity>0){      
      //c = mix_colors(c,QColor(168,35,35),::scale(patch->visual_note_intencity, MAX_NOTE_INTENCITY, 0, 0, 1));
      //QColor c = mix_colors(background_color,g_editor->colors[12],::scale(patch->visual_note_intencity, MAX_NOTE_INTENCITY, 0, 0, 1));
      QColor c = get_qcolor(NOTE_EVENT_INDICATOR_COLOR_NUM);
      c.setAlphaF(::scale(intencity, 0, MAX_NOTE_INTENCITY, 0.0, 1.0));
      painter->setPen(c);
      painter->setBrush(QBrush(c,Qt::SolidPattern));
      painter->drawRoundedRect(x1,y1,x2-x1,y2-y1,0.05,0.05);
    }

    // border
    QColor border_color = get_qcolor(NOTE_EVENT_INDICATOR_BORDER_COLOR_NUM);
    painter->setBrush(QBrush());
    painter->setPen(border_color);
    painter->drawRoundedRect(x1,y1,x2-x1,y2-y1,0.05,0.05);
  }


  // main box
  {
    int x1,y1,x2,y2;
    get_coordinates(x1,y1,x2,y2);

    //    if(is_current_patch==true){
      //}
    if (is_current_patch) { ///is_current_patch) {
      QColor c = get_qcolor(CURSOR_EDIT_ON_COLOR_NUM);
      c.setAlpha(150);
      painter->setPen(QPen(c, 3));
      painter->drawRoundedRect(x1-1,y1-1,x2-x1+1,y2-y1+1,7,7);
    } else {
      painter->setPen(QPen(border_color, 1));
      painter->drawRect(x1,y1,x2-x1,y2-y1);
    }
    //painter->fillRect(x1,y1,x2-x1,y2-y1);

    
    // Line between slider/buttons and name.
    {
      painter->setPen(QPen(border_color, 0.5));
      painter->drawLine(x1, y2-name_height, x2, y2-name_height);
    }

    // Greying if selected
    if (is_selected){// && !is_current_patch){
      QColor c(40,40,40,100);
      painter->fillRect(x1,y1,x2-x1,y2-y1,c);
    }
    
    // Bluing if autosuspending
    if (SP_is_autosuspending(plugin->sp)){
      //QColor c(0,0,60,80);
      QColor c = get_qcolor(MIXER_AUTOSUSPENSION_COLOR_NUM);
      //c.setAlpha(35);
      painter->fillRect(x1,y1,x2-x1,y2-y1,c);
    }
  }



  //printf("Paint Chip. Inputs: %d, Ouptuts: %d\n",_num_inputs,_num_outputs);

  // Ports
  {
    int x1,y1,x2,y2;
    get_coordinates(x1,y1,x2,y2);

    //painter->setPen(QPen(Qt::gray, 2));
    //QColor color(59,155,68,40);
    QColor color(80,90,80,20);
    painter->setPen(color);
    painter->setBrush(QBrush(color,Qt::SolidPattern));

    const int xborder = 5;
    const int yborder = 5;

    if(_num_inputs>0)
      painter->drawRoundedRect(xborder, y1+yborder,
                               port_width-2-xborder, y2-y1-yborder*2,
                               10,10);

    if(_num_outputs>0)
      painter->drawRoundedRect(x2+2, y1+yborder,
                               port_width-2-xborder, y2-y1-yborder*2,
                               10,10);

    painter->setBrush(QBrush());
  }
}

bool Chip::myMouseDoubleClickEvent (float x, float y) {

  if(SP_get_plugin(_sound_producer)->type->show_gui != NULL) {
    
    int x1,y1,x2,y2;
    get_name_coordinates(x1,y1,x2,y2);

    printf("I'm double clicked! %f,%f (%d,%d -> %d, %d)\n",x,y,x1,y1,x2,y2);

    if(x>x1 && x<x2 && y>y1 && y<y2){
      SP_get_plugin(_sound_producer)->type->show_gui(SP_get_plugin(_sound_producer));
      return true;
    }

  }
  //QGraphicsItem::mouseDoubleClickEvent(event);

  return false;
}

bool Chip::positionedAtSlider(QPointF pos){
  SoundPlugin *plugin = SP_get_plugin(_sound_producer);

  for(int i=0;i<2;i++){
    int x1,y1,x2,y2;
    if(i==0)
      get_slider1_coordinates(x1,y1,x2,y2);      
    else
      get_slider2_coordinates(x1,y1,x2,y2);      

    //printf("%d - %f - %d,    %d - %f - %d\n",x1,pos.x(),x2,y1,pos.y(),y2);

    if(pos.x()>x1 && pos.x()<x2 && pos.y()>y1 && pos.y()<y2){

      if(i==0 && plugin->type->num_inputs==0)
        continue;
      if(i==1 && plugin->type->num_outputs==0)
        continue;

      return true;
    }
  }

  return false;
}

void Chip::mousePressEvent(QGraphicsSceneMouseEvent *event)
{

  if(event->button()==Qt::LeftButton){

    SoundPlugin *plugin = SP_get_plugin(_sound_producer);
    int num_effects = plugin->type->num_effects;

    volatile struct Patch *patch = plugin->patch;
    R_ASSERT_RETURN_IF_FALSE(patch!=NULL);

    QPointF pos = event->pos();

    //printf("Pressed. %f / %f\n",pos.x(),pos.y());
    struct Instruments *instrument = get_audio_instrument();
    instrument->PP_Update(instrument,(struct Patch*)patch);

    if(event->modifiers() & Qt::ControlModifier)
      setSelected(true);
    else
      MW_set_selected_chip(this); //i.e. only set this one as the selected.

    // solo onoff
    {
      int x1,y1,x2,y2;
      get_solo_onoff_coordinates(x1,y1,x2,y2);
      if(pos.x()>x1 && pos.x()<x2 && pos.y()>y1 && pos.y()<y2){
        ADD_UNDO(AudioEffect_CurrPos((struct Patch*)patch, num_effects+EFFNUM_SOLO_ONOFF));

        //printf("Setting volume_is_on. Before: %d. After: %d\n",plugin->volume_is_on, !plugin->volume_is_on);
        float new_value = ATOMIC_GET(plugin->solo_is_on)?0.0f:1.0f;

        PLUGIN_set_effect_value(plugin, -1, num_effects+EFFNUM_SOLO_ONOFF, new_value, PLUGIN_NONSTORED_TYPE, PLUGIN_STORE_VALUE, FX_single);
        CHIP_update(plugin);
        GFX_update_instrument_widget((struct Patch*)patch);

        event->accept();
        return;
      }
    }

    // volume onoff
    {
      int x1,y1,x2,y2;
      get_volume_onoff_coordinates(x1,y1,x2,y2);
      if(pos.x()>x1 && pos.x()<x2 && pos.y()>y1 && pos.y()<y2){
        ADD_UNDO(AudioEffect_CurrPos((struct Patch*)patch, num_effects+EFFNUM_VOLUME_ONOFF));

        //printf("Setting volume_is_on. Before: %d. After: %d\n",plugin->volume_is_on, !plugin->volume_is_on);
        float new_value = ATOMIC_GET(plugin->volume_is_on)?0.0f:1.0f;

        PLUGIN_set_effect_value(plugin, -1, num_effects+EFFNUM_VOLUME_ONOFF, new_value, PLUGIN_NONSTORED_TYPE, PLUGIN_STORE_VALUE, FX_single);
        CHIP_update(plugin);
        GFX_update_instrument_widget((struct Patch*)patch);

        event->accept();
        return;
      }
    }

    // effects onoff
    {
      int x1,y1,x2,y2;
      get_effects_onoff_coordinates(x1,y1,x2,y2);
      if(pos.x()>x1 && pos.x()<x2 && pos.y()>y1 && pos.y()<y2){
        ADD_UNDO(AudioEffect_CurrPos((struct Patch*)patch, num_effects+EFFNUM_EFFECTS_ONOFF));

        float new_value = ATOMIC_GET(plugin->effects_are_on)?0.0f:1.0f;

        PLUGIN_set_effect_value(plugin, -1, num_effects+EFFNUM_EFFECTS_ONOFF, new_value, PLUGIN_NONSTORED_TYPE, PLUGIN_STORE_VALUE, FX_single);
        CHIP_update(plugin);
        GFX_update_instrument_widget((struct Patch*)patch);

        event->accept();
        return;
      }
    }

    for(int i=0;i<2;i++){
      int x1,y1,x2,y2;
      if(i==0)
        get_slider1_coordinates(x1,y1,x2,y2);      
      else
        get_slider2_coordinates(x1,y1,x2,y2);      

      //printf("Got it %d/%d - %d/%d. %d/%d/%d/%d\n",x1,y1,x2,y2,pos.x()>x1, pos.x()<x2, pos.y()>y1, pos.y()<y2);

      if(pos.x()>x1 && pos.x()<x2 && pos.y()>y1 && pos.y()<y2){
        int effect_num;

        if(i==0 && plugin->type->num_inputs==0)
          continue;
        if(i==1 && plugin->type->num_outputs==0)
          continue;

        if(i==0)
          effect_num = num_effects+EFFNUM_INPUT_VOLUME;
        else
          effect_num = num_effects+EFFNUM_VOLUME;

        ADD_UNDO(AudioEffect_CurrPos((struct Patch*)patch, effect_num));

        _slider_start_value = PLUGIN_get_effect_value(plugin, effect_num, VALUE_FROM_PLUGIN);
        _slider_start_pos = pos.x();
        /*
        float value = ::scale(pos.x(),x1,x2,0,1.0);
        PLUGIN_set_effect_value(plugin, -1, effect_num, value, PLUGIN_NONSTORED_TYPE, PLUGIN_STORE_VALUE, FX_single);

        CHIP_update(plugin);

        GFX_update_instrument_widget((struct Patch*)patch);
        */
        _slider_being_edited = i+1;
      }
    }

  }

  event->accept();
   
  //QGraphicsItem::mousePressEvent(event);
}

QVariant Chip::itemChange(GraphicsItemChange change, const QVariant &value) {
  if (change == ItemPositionHasChanged && this->scene()) {
    for (AudioConnection *connection : audio_connections)
      connection->update_position();
    for (EventConnection *connection : event_connections)
      connection->update_position();
    //printf("item pos changed\n");
  }

  return QGraphicsItem::itemChange(change, value);
}

void Chip::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
  if(_slider_being_edited>0){
    SoundPlugin *plugin = SP_get_plugin(_sound_producer);
    int num_effects = plugin->type->num_effects;

    QPointF pos = event->pos();

    int x1,y1,x2,y2;
    if(_slider_being_edited==0)
      get_slider1_coordinates(x1,y1,x2,y2);      
    else
      get_slider2_coordinates(x1,y1,x2,y2);      

    int effect_num;

    if(_slider_being_edited==1)
      effect_num = num_effects+EFFNUM_INPUT_VOLUME;
    else
      effect_num = num_effects+EFFNUM_VOLUME;

    bool ctrl_pressed = (event->modifiers() & Qt::ControlModifier);
    
    float delta = pos.x() - _slider_start_pos;
    if (ctrl_pressed)
      delta /= 10.0;
    
    float value = _slider_start_value + ::scale(delta,0,x2-x1,0,1.0);
    if(value>1.0)
      value=1.0;
    if(value<0.0)
      value=0.0;

    _slider_start_value = value;
    _slider_start_pos = pos.x();
    
    PLUGIN_set_effect_value(plugin, -1, effect_num, value, PLUGIN_NONSTORED_TYPE, PLUGIN_STORE_VALUE, FX_single);

    CHIP_update(plugin);
    volatile struct Patch *patch = plugin->patch;
    R_ASSERT_RETURN_IF_FALSE(patch!=NULL);

    GFX_update_instrument_widget((struct Patch*)patch);
  }

  event->accept();
  return;

  //QGraphicsItem::mouseMoveEvent(event);
}

void Chip::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
  if(_slider_being_edited>0)
    _slider_being_edited=0;

  event->accept();
}

struct Patch *CHIP_get_patch(Chip *chip){
  SoundPlugin *plugin = SP_get_plugin(chip->_sound_producer);
  volatile struct Patch *patch = plugin->patch;
  R_ASSERT(patch!=NULL);
  return (struct Patch*)patch;
}

Chip *CHIP_get(QGraphicsScene *scene, const Patch *patch){
  SoundPlugin *plugin = (SoundPlugin*)patch->patchdata;

  QList<QGraphicsItem *> das_items = scene->items();
  for (int i = 0; i < das_items.size(); ++i){
    Chip *chip = dynamic_cast<Chip*>(das_items.at(i));
    if(chip!=NULL){
      if(plugin==SP_get_plugin(chip->_sound_producer))
        return chip;
    }
  }
  
  return NULL;
}

static Chip *get_chip_from_patch_id(QGraphicsScene *scene, int64_t patch_id){
  struct Patch *patch = PATCH_get_from_id(patch_id);

  return CHIP_get(scene, patch);
}


static int64_t get_saving_patch_id(struct Patch *patch, const vector_t *patches){
  if (patches==NULL)
    return patch->id;

  for(int pos=0;pos<patches->num_elements;pos++)
    if(patches->elements[pos]==patch)
      return pos;

  RError("get_saving_patch_id: patch \"%s\" not found. patches length: %d",patch->name, patches->num_elements);
  
  return -1;
}
                            
hash_t *CONNECTION_get_state(const SuperConnection *connection, const vector_t *patches){
  hash_t *state=HASH_create(4);

  HASH_put_int(state, "from_patch", get_saving_patch_id(CHIP_get_patch(connection->from), patches));
  HASH_put_int(state, "to_patch",   get_saving_patch_id(CHIP_get_patch(connection->to),   patches));
  HASH_put_bool(state, "is_event_connection", connection->is_event_connection);

  return state;
}

void CONNECTION_create_from_state2(QGraphicsScene *scene, hash_t *state, int64_t patch_id_old, int64_t patch_id_new, int64_t patch_id_old2, int64_t patch_id_new2){
  int64_t id_from = HASH_get_int(state, "from_patch");
  int64_t id_to = HASH_get_int(state, "to_patch");

  if (id_from==patch_id_old)
    id_from = patch_id_new;
  
  if (id_to==patch_id_old)
    id_to = patch_id_new;
  
  if (id_from==patch_id_old2)
    id_from = patch_id_new2;
  
  if (id_to==patch_id_old2)
    id_to = patch_id_new2;
  
  Chip *from_chip = get_chip_from_patch_id(scene, id_from);
  Chip *to_chip   = get_chip_from_patch_id(scene, id_to);

  if(from_chip==NULL || to_chip==NULL) {
    RError("Could not find chip from patch id. %d: 0x%p, %d: 0x%p",HASH_get_int(state, "from_patch"),from_chip,HASH_get_int(state, "to_patch"),to_chip);
    return;
  }

  if(HASH_has_key(state, "is_event_connection") && HASH_get_bool(state, "is_event_connection")) // .rad files before 1.9.31 did not have even connections.
    CHIP_econnect_chips(scene, from_chip, to_chip);
  else
    CHIP_connect_chips(scene, from_chip, to_chip);
}

void CONNECTION_create_from_state(QGraphicsScene *scene, hash_t *state, int64_t patch_id_old, int64_t patch_id_new){
  CONNECTION_create_from_state2(scene, state, patch_id_old, patch_id_new, -1, -1);
}
