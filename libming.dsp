# Microsoft Developer Studio Project File - Name="libming" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Static Library" 0x0104

CFG=libming - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "libming.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "libming.mak" CFG="libming - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "libming - Win32 Release" (based on "Win32 (x86) Static Library")
!MESSAGE "libming - Win32 Debug" (based on "Win32 (x86) Static Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "libming - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /YX /FD /c
# ADD CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /D inline=__inline /YX /FD /c
# ADD BASE RSC /l 0x407 /d "NDEBUG" /l 0.907
# ADD RSC /l 0x407 /d "NDEBUG" /l 0.907
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ELSEIF  "$(CFG)" == "libming - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "_LIB" /YX /FD /GZ /c
# ADD CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "_LIB" /D inline=__inline /FR /YX /FD /GZ /c
# ADD BASE RSC /l 0x407 /d "_DEBUG" /l 0.907
# ADD RSC /l 0x407 /d "_DEBUG" /l 0.907
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ENDIF 

# Begin Target

# Name "libming - Win32 Release"
# Name "libming - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE="..\..\ming-0.2a\src\blocks\action.c"
# End Source File
# Begin Source File

SOURCE="..\..\ming-0.2a\src\blocks\action.h"
# End Source File
# Begin Source File

SOURCE="..\..\ming-0.2a\src\blocks\bitmap.c"
# End Source File
# Begin Source File

SOURCE="..\..\ming-0.2a\src\blocks\bitmap.h"
# End Source File
# Begin Source File

SOURCE="..\..\ming-0.2a\src\blocks\block.c"
# End Source File
# Begin Source File

SOURCE="..\..\ming-0.2a\src\blocks\block.h"
# End Source File
# Begin Source File

SOURCE="..\..\ming-0.2a\src\blocklist.c"
# End Source File
# Begin Source File

SOURCE="..\..\ming-0.2a\src\blocklist.h"
# End Source File
# Begin Source File

SOURCE="..\..\ming-0.2a\src\blocks\blocktypes.h"
# End Source File
# Begin Source File

SOURCE="..\..\ming-0.2a\src\blocktypes.h"
# End Source File
# Begin Source File

SOURCE="..\..\ming-0.2a\src\blocks\browserfont.c"
# End Source File
# Begin Source File

SOURCE="..\..\ming-0.2a\src\blocks\browserfont.h"
# End Source File
# Begin Source File

SOURCE="..\..\ming-0.2a\src\blocks\button.c"
# End Source File
# Begin Source File

SOURCE="..\..\ming-0.2a\src\blocks\button.h"
# End Source File
# Begin Source File

SOURCE="..\..\ming-0.2a\src\blocks\character.c"
# End Source File
# Begin Source File

SOURCE="..\..\ming-0.2a\src\blocks\character.h"
# End Source File
# Begin Source File

SOURCE="..\..\ming-0.2a\src\blocks\cxform.c"
# End Source File
# Begin Source File

SOURCE="..\..\ming-0.2a\src\blocks\cxform.h"
# End Source File
# Begin Source File

SOURCE="..\..\ming-0.2a\src\blocks\dbl.c"
# End Source File
# Begin Source File

SOURCE="..\..\ming-0.2a\src\blocks\dbl.h"
# End Source File
# Begin Source File

SOURCE="..\..\ming-0.2a\src\displaylist.c"
# End Source File
# Begin Source File

SOURCE="..\..\ming-0.2a\src\displaylist.h"
# End Source File
# Begin Source File

SOURCE="..\..\ming-0.2a\src\blocks\error.c"
# End Source File
# Begin Source File

SOURCE="..\..\ming-0.2a\src\blocks\error.h"
# End Source File
# Begin Source File

SOURCE="..\..\ming-0.2a\src\fill.c"
# End Source File
# Begin Source File

SOURCE="..\..\ming-0.2a\src\fill.h"
# End Source File
# Begin Source File

SOURCE="..\..\ming-0.2a\src\blocks\fillstyle.c"
# End Source File
# Begin Source File

SOURCE="..\..\ming-0.2a\src\blocks\fillstyle.h"
# End Source File
# Begin Source File

SOURCE="..\..\ming-0.2a\src\blocks\font.c"
# End Source File
# Begin Source File

SOURCE="..\..\ming-0.2a\src\blocks\font.h"
# End Source File
# Begin Source File

SOURCE="..\..\ming-0.2a\src\blocks\fontinfo.c"
# End Source File
# Begin Source File

SOURCE="..\..\ming-0.2a\src\blocks\fontinfo.h"
# End Source File
# Begin Source File

SOURCE="..\..\ming-0.2a\src\blocks\gradient.c"
# End Source File
# Begin Source File

SOURCE="..\..\ming-0.2a\src\blocks\gradient.h"
# End Source File
# Begin Source File

SOURCE="..\..\ming-0.2a\src\blocks\input.c"
# End Source File
# Begin Source File

SOURCE="..\..\ming-0.2a\src\blocks\input.h"
# End Source File
# Begin Source File

SOURCE="..\..\ming-0.2a\src\blocks\jpeg.c"
# End Source File
# Begin Source File

SOURCE="..\..\ming-0.2a\src\blocks\jpeg.h"
# End Source File
# Begin Source File

SOURCE="..\..\ming-0.2a\src\libswf.h"
# End Source File
# Begin Source File

SOURCE="..\..\ming-0.2a\src\blocks\libming.h"
# End Source File
# Begin Source File

SOURCE="..\..\ming-0.2a\src\blocks\linestyle.c"
# End Source File
# Begin Source File

SOURCE="..\..\ming-0.2a\src\blocks\linestyle.h"
# End Source File
# Begin Source File

SOURCE="..\..\ming-0.2a\src\blocks\loadfont.c"
# End Source File
# Begin Source File

SOURCE="..\..\ming-0.2a\src\blocks\matrix.c"
# End Source File
# Begin Source File

SOURCE="..\..\ming-0.2a\src\blocks\matrix.h"
# End Source File
# Begin Source File

SOURCE="..\..\ming-0.2a\src\blocks\method.c"
# End Source File
# Begin Source File

SOURCE="..\..\ming-0.2a\src\blocks\method.h"
# End Source File
# Begin Source File

SOURCE="..\..\ming-0.2a\src\ming.c"
# End Source File
# Begin Source File

SOURCE="..\..\ming-0.2a\src\ming.h"
# End Source File
# Begin Source File

SOURCE="..\..\ming-0.2a\src\blocks\morph.c"
# End Source File
# Begin Source File

SOURCE="..\..\ming-0.2a\src\blocks\morph.h"
# End Source File
# Begin Source File

SOURCE="..\..\ming-0.2a\src\movie.c"
# End Source File
# Begin Source File

SOURCE="..\..\ming-0.2a\src\movie.h"
# End Source File
# Begin Source File

SOURCE="..\..\ming-0.2a\src\movieclip.c"
# End Source File
# Begin Source File

SOURCE="..\..\ming-0.2a\src\movieclip.h"
# End Source File
# Begin Source File

SOURCE="..\..\ming-0.2a\src\blocks\mp3.c"
# End Source File
# Begin Source File

SOURCE="..\..\ming-0.2a\src\blocks\output.c"
# End Source File
# Begin Source File

SOURCE="..\..\ming-0.2a\src\blocks\output.h"
# End Source File
# Begin Source File

SOURCE="..\..\ming-0.2a\src\blocks\outputblock.c"
# End Source File
# Begin Source File

SOURCE="..\..\ming-0.2a\src\blocks\outputblock.h"
# End Source File
# Begin Source File

SOURCE="..\..\ming-0.2a\src\blocks\placeobject.c"
# End Source File
# Begin Source File

SOURCE="..\..\ming-0.2a\src\position.c"
# End Source File
# Begin Source File

SOURCE="..\..\ming-0.2a\src\position.h"
# End Source File
# Begin Source File

SOURCE="..\..\ming-0.2a\src\blocks\rect.c"
# End Source File
# Begin Source File

SOURCE="..\..\ming-0.2a\src\blocks\rect.h"
# End Source File
# Begin Source File

SOURCE="..\..\ming-0.2a\src\blocks\shape.c"
# End Source File
# Begin Source File

SOURCE="..\..\ming-0.2a\src\blocks\shape.h"
# End Source File
# Begin Source File

SOURCE="..\..\ming-0.2a\src\shape_cubic.c"
# End Source File
# Begin Source File

SOURCE="..\..\ming-0.2a\src\shape_util.c"
# End Source File
# Begin Source File

SOURCE="..\..\ming-0.2a\src\shape_util.h"
# End Source File
# Begin Source File

SOURCE="..\..\ming-0.2a\src\blocks\sound.c"
# End Source File
# Begin Source File

SOURCE="..\..\ming-0.2a\src\blocks\sound.h"
# End Source File
# Begin Source File

SOURCE="..\..\ming-0.2a\src\blocks\soundstream.c"
# End Source File
# Begin Source File

SOURCE="..\..\ming-0.2a\src\blocks\soundstream.h"
# End Source File
# Begin Source File

SOURCE="..\..\ming-0.2a\src\blocks\sprite.c"
# End Source File
# Begin Source File

SOURCE="..\..\ming-0.2a\src\blocks\sprite.h"
# End Source File
# Begin Source File

SOURCE="..\..\ming-0.2a\src\blocks\swf.h"
# End Source File
# Begin Source File

SOURCE="..\..\ming-0.2a\src\blocks\text.c"
# End Source File
# Begin Source File

SOURCE="..\..\ming-0.2a\src\blocks\text.h"
# End Source File
# Begin Source File

SOURCE="..\..\ming-0.2a\src\text_util.c"
# End Source File
# Begin Source File

SOURCE="..\..\ming-0.2a\src\text_util.h"
# End Source File
# Begin Source File

SOURCE="..\..\ming-0.2a\src\blocks\textfield.c"
# End Source File
# Begin Source File

SOURCE="..\..\ming-0.2a\src\blocks\textfield.h"
# End Source File
# Begin Source File

SOURCE="..\..\ming-0.2a\src\blocks\ttffont.c"
# End Source File
# Begin Source File

SOURCE="..\..\ming-0.2a\src\blocks\ttffont.h"
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE="..\..\ming-0.2a\mingpp.h"
# End Source File
# End Group
# End Target
# End Project
