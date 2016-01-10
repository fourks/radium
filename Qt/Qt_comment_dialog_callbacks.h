/* Copyright 2013 Kjetil S. Matheussen

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


#include <stdio.h>
#include <sndfile.h>
#include <unistd.h>

#include <QMessageBox>
#include <QFileDialog>
#include <FocusSniffers.h>

#include "../common/nsmtracker.h"
#include "../common/hashmap_proc.h"
#include "../common/OS_string_proc.h"

//namespace{
#include "Qt_MyQCheckBox.h"
//}

#include "helpers.h"

#include "Qt_comment_dialog.h"


class comment_dialog : public QDialog, public Ui::Comment_dialog {
  Q_OBJECT

 public:
  bool _initing;

 comment_dialog(QWidget *parent=NULL)
    : QDialog(parent)
  {
    _initing = true;

    setupUi(this);

    _initing = false;
  }

public slots:

  void on_buttonBox_clicked(QAbstractButton * button){
    this->hide();
  }

};
