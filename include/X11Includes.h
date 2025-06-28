#pragma once
#ifndef X11INCLUDES_H
#define X11INCLUDES_H

// Salvar definições originais de Bool se existirem
#ifdef Bool
#define X11_Bool_Defined
#define X11_Bool_Value Bool
#undef Bool
#endif

// Salvar definições originais de Status se existirem
#ifdef Status
#define X11_Status_Defined
#define X11_Status_Value Status
#undef Status
#endif

// Salvar definições originais de None se existirem
#ifdef None
#define X11_None_Defined
#define X11_None_Value None
#undef None
#endif

// Salvar definições originais de Unsorted se existirem
#ifdef Unsorted
#define X11_Unsorted_Defined
#define X11_Unsorted_Value Unsorted
#undef Unsorted
#endif

// Incluir os headers X11
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/Xutil.h>
#include <X11/extensions/Xrandr.h>
#include <X11/extensions/shape.h>
#include <xcb/xcb.h>

// Outras dependências do sistema
#include <unistd.h>
#include <sys/types.h>
#include <signal.h>
#include <pwd.h>

// Tipos do X11 que precisamos preservar
using X11Bool = Bool;
using X11Status = Status;
using X11Window = Window;
using X11Display = Display;

// Remover definições do X11 para evitar conflitos com Qt
#undef Bool
#undef Status
//#undef None
#undef Unsorted

#endif // X11INCLUDES_H 