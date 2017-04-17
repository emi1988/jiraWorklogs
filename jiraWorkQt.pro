#-------------------------------------------------
#
# Project created by QtCreator 2017-04-13T10:42:52
#
#-------------------------------------------------

QT       += core gui
QT += network

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = jiraWorkQt
TEMPLATE = app


SOURCES += main.cpp\
        mainwindow.cpp \
    dialoguserdata.cpp \
    simplecrypt.cpp

HEADERS  += mainwindow.h \
    dialoguserdata.h \
    simplecrypt.h

FORMS    += mainwindow.ui \
    dialoguserdata.ui
