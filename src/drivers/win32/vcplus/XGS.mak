# Microsoft Developer Studio Generated NMAKE File, Format Version 4.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Application" 0x0101

!IF "$(CFG)" == ""
CFG=XGS - Win32 Debug
!MESSAGE No configuration specified.  Defaulting to XGS - Win32 Debug.
!ENDIF 

!IF "$(CFG)" != "XGS - Win32 Release" && "$(CFG)" != "XGS - Win32 Debug"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE on this makefile
!MESSAGE by defining the macro CFG on the command line.  For example:
!MESSAGE 
!MESSAGE NMAKE /f "XGS.mak" CFG="XGS - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "XGS - Win32 Release" (based on "Win32 (x86) Application")
!MESSAGE "XGS - Win32 Debug" (based on "Win32 (x86) Application")
!MESSAGE 
!ERROR An invalid configuration is specified.
!ENDIF 

!IF "$(OS)" == "Windows_NT"
NULL=
!ELSE 
NULL=nul
!ENDIF 
################################################################################
# Begin Project
# PROP Target_Last_Scanned "XGS - Win32 Debug"
RSC=rc.exe
MTL=mktyplib.exe
CPP=cl.exe

!IF  "$(CFG)" == "XGS - Win32 Release"

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
OUTDIR=.\Release
INTDIR=.\Release

ALL : "$(OUTDIR)\XGS.exe"

CLEAN : 
	-@erase ".\Release\XGS.exe"
	-@erase ".\Release\hires.obj"
	-@erase ".\Release\emul.obj"
	-@erase ".\Release\adb.obj"
	-@erase ".\Release\fontutil.obj"
	-@erase ".\Release\clock.obj"
	-@erase ".\Release\opcodes1.obj"
	-@erase ".\Release\debugger.obj"
	-@erase ".\Release\disks.obj"
	-@erase ".\Release\opcodes5.obj"
	-@erase ".\Release\snd-drv.obj"
	-@erase ".\Release\rw.obj"
	-@erase ".\Release\memory.obj"
	-@erase ".\Release\adb-drv.obj"
	-@erase ".\Release\opcodes4.obj"
	-@erase ".\Release\Main.obj"
	-@erase ".\Release\cpu.obj"
	-@erase ".\Release\video.obj"
	-@erase ".\Release\smtport.obj"
	-@erase ".\Release\lores.obj"
	-@erase ".\Release\opcodes3.obj"
	-@erase ".\Release\vid-drv.obj"
	-@erase ".\Release\sound.obj"
	-@erase ".\Release\super.obj"
	-@erase ".\Release\text.obj"
	-@erase ".\Release\iwm.obj"
	-@erase ".\Release\dispatch.obj"
	-@erase ".\Release\opcodes2.obj"
	-@erase ".\Release\table.obj"
	-@erase ".\Release\XGS.res"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

# ADD BASE CPP /nologo /W3 /GX /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /YX /c
# ADD CPP /nologo /G5 /Gr /W3 /GX /O2 /I "INCLUDE" /I "ARCH\WIN32" /I "ARCH\GENERIC" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /YX /c
# SUBTRACT CPP /X /u
CPP_PROJ=/nologo /G5 /Gr /ML /W3 /GX /O2 /I "INCLUDE" /I "ARCH\WIN32" /I\
 "ARCH\GENERIC" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /Fp"$(INTDIR)/XGS.pch" /YX\
 /Fo"$(INTDIR)/" /c 
CPP_OBJS=.\Release/
CPP_SBRS=
# ADD BASE MTL /nologo /D "NDEBUG" /win32
# ADD MTL /nologo /D "NDEBUG" /win32
MTL_PROJ=/nologo /D "NDEBUG" /win32 
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
RSC_PROJ=/l 0x409 /fo"$(INTDIR)/XGS.res" /d "NDEBUG" 
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
BSC32_FLAGS=/nologo /o"$(OUTDIR)/XGS.bsc" 
BSC32_SBRS=
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /machine:I386
# ADD LINK32 libc.lib kernel32.lib user32.lib gdi32.lib comdlg32.lib advapi32.lib winmm.lib ddraw.lib binmode.obj /nologo /subsystem:windows /machine:I386
# SUBTRACT LINK32 /incremental:yes /debug /nodefaultlib
LINK32_FLAGS=libc.lib kernel32.lib user32.lib gdi32.lib comdlg32.lib\
 advapi32.lib winmm.lib ddraw.lib binmode.obj /nologo /subsystem:windows\
 /incremental:no /pdb:"$(OUTDIR)/XGS.pdb" /machine:I386 /out:"$(OUTDIR)/XGS.exe"\
 
LINK32_OBJS= \
	"$(INTDIR)/hires.obj" \
	"$(INTDIR)/emul.obj" \
	"$(INTDIR)/adb.obj" \
	"$(INTDIR)/fontutil.obj" \
	"$(INTDIR)/clock.obj" \
	"$(INTDIR)/opcodes1.obj" \
	"$(INTDIR)/debugger.obj" \
	"$(INTDIR)/disks.obj" \
	"$(INTDIR)/opcodes5.obj" \
	"$(INTDIR)/snd-drv.obj" \
	"$(INTDIR)/rw.obj" \
	"$(INTDIR)/memory.obj" \
	"$(INTDIR)/adb-drv.obj" \
	"$(INTDIR)/opcodes4.obj" \
	"$(INTDIR)/Main.obj" \
	"$(INTDIR)/cpu.obj" \
	"$(INTDIR)/video.obj" \
	"$(INTDIR)/smtport.obj" \
	"$(INTDIR)/lores.obj" \
	"$(INTDIR)/opcodes3.obj" \
	"$(INTDIR)/vid-drv.obj" \
	"$(INTDIR)/sound.obj" \
	"$(INTDIR)/super.obj" \
	"$(INTDIR)/text.obj" \
	"$(INTDIR)/iwm.obj" \
	"$(INTDIR)/dispatch.obj" \
	"$(INTDIR)/opcodes2.obj" \
	"$(INTDIR)/table.obj" \
	"$(INTDIR)/XGS.res"

"$(OUTDIR)\XGS.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "XGS - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "XGS___Wi"
# PROP BASE Intermediate_Dir "XGS___Wi"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "XGS___Wi"
# PROP Intermediate_Dir "XGS___Wi"
# PROP Target_Dir ""
OUTDIR=.\XGS___Wi
INTDIR=.\XGS___Wi

ALL : "$(OUTDIR)\XGS.exe"

CLEAN : 
	-@erase ".\XGS___Wi\vc40.pdb"
	-@erase ".\XGS___Wi\vc40.idb"
	-@erase ".\XGS___Wi\XGS.exe"
	-@erase ".\XGS___Wi\smtport.obj"
	-@erase ".\XGS___Wi\video.obj"
	-@erase ".\XGS___Wi\opcodes1.obj"
	-@erase ".\XGS___Wi\debugger.obj"
	-@erase ".\XGS___Wi\opcodes5.obj"
	-@erase ".\XGS___Wi\cpu.obj"
	-@erase ".\XGS___Wi\memory.obj"
	-@erase ".\XGS___Wi\lores.obj"
	-@erase ".\XGS___Wi\sound.obj"
	-@erase ".\XGS___Wi\super.obj"
	-@erase ".\XGS___Wi\snd-drv.obj"
	-@erase ".\XGS___Wi\opcodes4.obj"
	-@erase ".\XGS___Wi\table.obj"
	-@erase ".\XGS___Wi\iwm.obj"
	-@erase ".\XGS___Wi\rw.obj"
	-@erase ".\XGS___Wi\opcodes3.obj"
	-@erase ".\XGS___Wi\clock.obj"
	-@erase ".\XGS___Wi\text.obj"
	-@erase ".\XGS___Wi\disks.obj"
	-@erase ".\XGS___Wi\dispatch.obj"
	-@erase ".\XGS___Wi\adb.obj"
	-@erase ".\XGS___Wi\vid-drv.obj"
	-@erase ".\XGS___Wi\Main.obj"
	-@erase ".\XGS___Wi\opcodes2.obj"
	-@erase ".\XGS___Wi\adb-drv.obj"
	-@erase ".\XGS___Wi\emul.obj"
	-@erase ".\XGS___Wi\hires.obj"
	-@erase ".\XGS___Wi\fontutil.obj"
	-@erase ".\XGS___Wi\XGS.res"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

# ADD BASE CPP /nologo /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /YX /c
# ADD CPP /nologo /G5 /Gr /ML /W3 /Gm /GX /Zi /Od /I "INCLUDE" /I "ARCH\WIN32" /I "ARCH\GENERIC" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /YX /c
# SUBTRACT CPP /X /u
CPP_PROJ=/nologo /G5 /Gr /ML /W3 /Gm /GX /Zi /Od /I "INCLUDE" /I "ARCH\WIN32"\
 /I "ARCH\GENERIC" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /Fp"$(INTDIR)/XGS.pch"\
 /YX /Fo"$(INTDIR)/" /Fd"$(INTDIR)/" /c 
CPP_OBJS=.\XGS___Wi/
CPP_SBRS=
# ADD BASE MTL /nologo /D "_DEBUG" /win32
# ADD MTL /nologo /D "_DEBUG" /win32
MTL_PROJ=/nologo /D "_DEBUG" /win32 
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
RSC_PROJ=/l 0x409 /fo"$(INTDIR)/XGS.res" /d "_DEBUG" 
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
BSC32_FLAGS=/nologo /o"$(OUTDIR)/XGS.bsc" 
BSC32_SBRS=
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /debug /machine:I386
# ADD LINK32 libc.lib kernel32.lib user32.lib gdi32.lib comdlg32.lib advapi32.lib winmm.lib ddraw.lib binmode.obj /nologo /subsystem:windows /incremental:no /machine:I386
# SUBTRACT LINK32 /debug /nodefaultlib
LINK32_FLAGS=libc.lib kernel32.lib user32.lib gdi32.lib comdlg32.lib\
 advapi32.lib winmm.lib ddraw.lib binmode.obj /nologo /subsystem:windows\
 /incremental:no /pdb:"$(OUTDIR)/XGS.pdb" /machine:I386 /out:"$(OUTDIR)/XGS.exe"\
 
LINK32_OBJS= \
	"$(INTDIR)/smtport.obj" \
	"$(INTDIR)/video.obj" \
	"$(INTDIR)/opcodes1.obj" \
	"$(INTDIR)/debugger.obj" \
	"$(INTDIR)/opcodes5.obj" \
	"$(INTDIR)/cpu.obj" \
	"$(INTDIR)/memory.obj" \
	"$(INTDIR)/lores.obj" \
	"$(INTDIR)/sound.obj" \
	"$(INTDIR)/super.obj" \
	"$(INTDIR)/snd-drv.obj" \
	"$(INTDIR)/opcodes4.obj" \
	"$(INTDIR)/table.obj" \
	"$(INTDIR)/iwm.obj" \
	"$(INTDIR)/rw.obj" \
	"$(INTDIR)/opcodes3.obj" \
	"$(INTDIR)/clock.obj" \
	"$(INTDIR)/text.obj" \
	"$(INTDIR)/disks.obj" \
	"$(INTDIR)/dispatch.obj" \
	"$(INTDIR)/adb.obj" \
	"$(INTDIR)/vid-drv.obj" \
	"$(INTDIR)/Main.obj" \
	"$(INTDIR)/opcodes2.obj" \
	"$(INTDIR)/adb-drv.obj" \
	"$(INTDIR)/emul.obj" \
	"$(INTDIR)/hires.obj" \
	"$(INTDIR)/fontutil.obj" \
	"$(INTDIR)/XGS.res"

"$(OUTDIR)\XGS.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ENDIF 

.c{$(CPP_OBJS)}.obj:
   $(CPP) $(CPP_PROJ) $<  

.cpp{$(CPP_OBJS)}.obj:
   $(CPP) $(CPP_PROJ) $<  

.cxx{$(CPP_OBJS)}.obj:
   $(CPP) $(CPP_PROJ) $<  

.c{$(CPP_SBRS)}.sbr:
   $(CPP) $(CPP_PROJ) $<  

.cpp{$(CPP_SBRS)}.sbr:
   $(CPP) $(CPP_PROJ) $<  

.cxx{$(CPP_SBRS)}.sbr:
   $(CPP) $(CPP_PROJ) $<  

################################################################################
# Begin Target

# Name "XGS - Win32 Release"
# Name "XGS - Win32 Debug"

!IF  "$(CFG)" == "XGS - Win32 Release"

!ELSEIF  "$(CFG)" == "XGS - Win32 Debug"

!ENDIF 

################################################################################
# Begin Source File

SOURCE=.\XGS.rc

"$(INTDIR)\XGS.res" : $(SOURCE) "$(INTDIR)"
   $(RSC) $(RSC_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\video.c
DEP_CPP_VIDEO=\
	".\INCLUDE\xgs.h"\
	".\INCLUDE\emul.h"\
	".\INCLUDE\video.h"\
	".\ARCH\WIN32\vid-drv.h"\
	".\ARCH\WIN32\config.h"\
	".\INCLUDE\cpu.h"\
	

"$(INTDIR)\video.obj" : $(SOURCE) $(DEP_CPP_VIDEO) "$(INTDIR)"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\clock.c
DEP_CPP_CLOCK=\
	".\INCLUDE\xgs.h"\
	".\INCLUDE\clock.h"\
	".\INCLUDE\emul.h"\
	".\INCLUDE\video.h"\
	".\ARCH\WIN32\config.h"\
	".\INCLUDE\cpu.h"\
	

"$(INTDIR)\clock.obj" : $(SOURCE) $(DEP_CPP_CLOCK) "$(INTDIR)"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\cpu.c
DEP_CPP_CPU_C=\
	".\ARCH\WIN32\config.h"\
	".\INCLUDE\cpu.h"\
	".\INCLUDE\emul.h"\
	

"$(INTDIR)\cpu.obj" : $(SOURCE) $(DEP_CPP_CPU_C) "$(INTDIR)"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\debugger.c
DEP_CPP_DEBUG=\
	".\ARCH\WIN32\config.h"\
	".\INCLUDE\cpu.h"\
	".\INCLUDE\cpumicro.h"\
	".\INCLUDE\emul.h"\
	

"$(INTDIR)\debugger.obj" : $(SOURCE) $(DEP_CPP_DEBUG) "$(INTDIR)"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\disks.c
DEP_CPP_DISKS=\
	".\INCLUDE\xgs.h"\
	".\INCLUDE\disks.h"\
	".\ARCH\WIN32\config.h"\
	".\INCLUDE\cpu.h"\
	

"$(INTDIR)\disks.obj" : $(SOURCE) $(DEP_CPP_DISKS) "$(INTDIR)"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\dispatch.c
DEP_CPP_DISPA=\
	".\ARCH\WIN32\config.h"\
	".\INCLUDE\cpu.h"\
	".\INCLUDE\cpumicro.h"\
	".\INCLUDE\emul.h"\
	

"$(INTDIR)\dispatch.obj" : $(SOURCE) $(DEP_CPP_DISPA) "$(INTDIR)"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\emul.c
DEP_CPP_EMUL_=\
	".\INCLUDE\xgs.h"\
	".\INCLUDE\adb.h"\
	".\INCLUDE\clock.h"\
	".\INCLUDE\disks.h"\
	".\INCLUDE\emul.h"\
	".\INCLUDE\iwm.h"\
	".\ARCH\WIN32\main.h"\
	".\INCLUDE\smtport.h"\
	".\INCLUDE\sound.h"\
	".\INCLUDE\video.h"\
	".\ARCH\WIN32\vid-drv.h"\
	".\INCLUDE\cpu.h"\
	".\ARCH\WIN32\config.h"\
	

"$(INTDIR)\emul.obj" : $(SOURCE) $(DEP_CPP_EMUL_) "$(INTDIR)"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\fontutil.c
DEP_CPP_FONTU=\
	".\INCLUDE\xgs.h"\
	".\ARCH\WIN32\config.h"\
	".\INCLUDE\cpu.h"\
	

"$(INTDIR)\fontutil.obj" : $(SOURCE) $(DEP_CPP_FONTU) "$(INTDIR)"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\hires.c
DEP_CPP_HIRES=\
	".\INCLUDE\xgs.h"\
	".\INCLUDE\video.h"\
	".\ARCH\WIN32\vid-drv.h"\
	".\ARCH\WIN32\config.h"\
	".\INCLUDE\cpu.h"\
	

"$(INTDIR)\hires.obj" : $(SOURCE) $(DEP_CPP_HIRES) "$(INTDIR)"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\iwm.c
DEP_CPP_IWM_C=\
	".\INCLUDE\xgs.h"\
	".\INCLUDE\disks.h"\
	".\INCLUDE\emul.h"\
	".\INCLUDE\iwm.h"\
	".\ARCH\WIN32\config.h"\
	".\INCLUDE\cpu.h"\
	

"$(INTDIR)\iwm.obj" : $(SOURCE) $(DEP_CPP_IWM_C) "$(INTDIR)"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\lores.c
DEP_CPP_LORES=\
	".\INCLUDE\xgs.h"\
	".\INCLUDE\video.h"\
	".\ARCH\WIN32\vid-drv.h"\
	".\ARCH\WIN32\config.h"\
	".\INCLUDE\cpu.h"\
	

"$(INTDIR)\lores.obj" : $(SOURCE) $(DEP_CPP_LORES) "$(INTDIR)"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\memory.c
DEP_CPP_MEMOR=\
	".\INCLUDE\xgs.h"\
	{$(INCLUDE)}"\sys\Types.h"\
	{$(INCLUDE)}"\sys\Stat.h"\
	".\INCLUDE\adb.h"\
	".\INCLUDE\disks.h"\
	".\INCLUDE\emul.h"\
	".\INCLUDE\iwm.h"\
	".\INCLUDE\smtport.h"\
	".\INCLUDE\video.h"\
	".\ARCH\WIN32\config.h"\
	".\INCLUDE\cpu.h"\
	

"$(INTDIR)\memory.obj" : $(SOURCE) $(DEP_CPP_MEMOR) "$(INTDIR)"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\opcodes1.c
DEP_CPP_OPCOD=\
	".\INCLUDE\opcodes.h"\
	".\ARCH\WIN32\config.h"\
	".\INCLUDE\cpu.h"\
	".\INCLUDE\cpumacro.h"\
	".\INCLUDE\cpumicro.h"\
	".\INCLUDE\emul.h"\
	".\INCLUDE\cycles.h"\
	

"$(INTDIR)\opcodes1.obj" : $(SOURCE) $(DEP_CPP_OPCOD) "$(INTDIR)"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\opcodes2.c
DEP_CPP_OPCODE=\
	".\INCLUDE\opcodes.h"\
	".\ARCH\WIN32\config.h"\
	".\INCLUDE\cpu.h"\
	".\INCLUDE\cpumacro.h"\
	".\INCLUDE\cpumicro.h"\
	".\INCLUDE\emul.h"\
	".\INCLUDE\cycles.h"\
	

"$(INTDIR)\opcodes2.obj" : $(SOURCE) $(DEP_CPP_OPCODE) "$(INTDIR)"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\opcodes3.c
DEP_CPP_OPCODES=\
	".\INCLUDE\opcodes.h"\
	".\ARCH\WIN32\config.h"\
	".\INCLUDE\cpu.h"\
	".\INCLUDE\cpumacro.h"\
	".\INCLUDE\cpumicro.h"\
	".\INCLUDE\emul.h"\
	".\INCLUDE\cycles.h"\
	

"$(INTDIR)\opcodes3.obj" : $(SOURCE) $(DEP_CPP_OPCODES) "$(INTDIR)"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\opcodes4.c
DEP_CPP_OPCODES4=\
	".\INCLUDE\opcodes.h"\
	".\ARCH\WIN32\config.h"\
	".\INCLUDE\cpu.h"\
	".\INCLUDE\cpumacro.h"\
	".\INCLUDE\cpumicro.h"\
	".\INCLUDE\emul.h"\
	".\INCLUDE\cycles.h"\
	

"$(INTDIR)\opcodes4.obj" : $(SOURCE) $(DEP_CPP_OPCODES4) "$(INTDIR)"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\opcodes5.c
DEP_CPP_OPCODES5=\
	".\INCLUDE\opcodes.h"\
	".\ARCH\WIN32\config.h"\
	".\INCLUDE\cpu.h"\
	".\INCLUDE\cpumacro.h"\
	".\INCLUDE\cpumicro.h"\
	".\INCLUDE\emul.h"\
	".\INCLUDE\cycles.h"\
	

"$(INTDIR)\opcodes5.obj" : $(SOURCE) $(DEP_CPP_OPCODES5) "$(INTDIR)"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\rw.c
DEP_CPP_RW_C22=\
	".\INCLUDE\xgs.h"\
	".\INCLUDE\adb.h"\
	".\INCLUDE\clock.h"\
	".\INCLUDE\disks.h"\
	".\INCLUDE\emul.h"\
	".\INCLUDE\iwm.h"\
	".\INCLUDE\smtport.h"\
	".\INCLUDE\sound.h"\
	".\INCLUDE\video.h"\
	".\ARCH\WIN32\config.h"\
	".\INCLUDE\cpu.h"\
	

"$(INTDIR)\rw.obj" : $(SOURCE) $(DEP_CPP_RW_C22) "$(INTDIR)"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\smtport.c
DEP_CPP_SMTPO=\
	".\INCLUDE\xgs.h"\
	".\INCLUDE\disks.h"\
	".\INCLUDE\emul.h"\
	".\INCLUDE\smtport.h"\
	".\ARCH\WIN32\config.h"\
	".\INCLUDE\cpu.h"\
	

"$(INTDIR)\smtport.obj" : $(SOURCE) $(DEP_CPP_SMTPO) "$(INTDIR)"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\sound.c
DEP_CPP_SOUND=\
	".\INCLUDE\xgs.h"\
	".\INCLUDE\emul.h"\
	".\INCLUDE\sound.h"\
	".\ARCH\WIN32\snd-drv.h"\
	".\ARCH\WIN32\config.h"\
	".\INCLUDE\cpu.h"\
	

"$(INTDIR)\sound.obj" : $(SOURCE) $(DEP_CPP_SOUND) "$(INTDIR)"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\super.c
DEP_CPP_SUPER=\
	".\INCLUDE\xgs.h"\
	".\INCLUDE\video.h"\
	".\ARCH\WIN32\vid-drv.h"\
	".\ARCH\WIN32\config.h"\
	".\INCLUDE\cpu.h"\
	

"$(INTDIR)\super.obj" : $(SOURCE) $(DEP_CPP_SUPER) "$(INTDIR)"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\table.c
DEP_CPP_TABLE=\
	".\INCLUDE\cputable.h"\
	

"$(INTDIR)\table.obj" : $(SOURCE) $(DEP_CPP_TABLE) "$(INTDIR)"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\text.c
DEP_CPP_TEXT_=\
	".\INCLUDE\xgs.h"\
	".\INCLUDE\video.h"\
	".\ARCH\WIN32\vid-drv.h"\
	".\ARCH\WIN32\config.h"\
	".\INCLUDE\cpu.h"\
	

"$(INTDIR)\text.obj" : $(SOURCE) $(DEP_CPP_TEXT_) "$(INTDIR)"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\adb.c
DEP_CPP_ADB_C=\
	".\INCLUDE\xgs.h"\
	".\INCLUDE\adb.h"\
	".\ARCH\WIN32\adb-drv.h"\
	".\INCLUDE\emul.h"\
	".\INCLUDE\video.h"\
	".\INCLUDE\cpu.h"\
	".\ARCH\WIN32\config.h"\
	

"$(INTDIR)\adb.obj" : $(SOURCE) $(DEP_CPP_ADB_C) "$(INTDIR)"


# End Source File
################################################################################
# Begin Source File

SOURCE=".\arch\win32\vid-drv.cpp"
DEP_CPP_VID_D=\
	{$(INCLUDE)}"\ddraw.h"\
	".\ARCH\WIN32\main.h"\
	".\INCLUDE\xgs.h"\
	".\INCLUDE\video.h"\
	".\ARCH\WIN32\vid-drv.h"\
	".\ARCH\WIN32\config.h"\
	".\INCLUDE\cpu.h"\
	

"$(INTDIR)\vid-drv.obj" : $(SOURCE) $(DEP_CPP_VID_D) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\arch\win32\Main.c
DEP_CPP_MAIN_=\
	{$(INCLUDE)}"\sys\Stat.h"\
	".\INCLUDE\xgs.h"\
	".\INCLUDE\adb.h"\
	".\ARCH\WIN32\adb-drv.h"\
	".\INCLUDE\disks.h"\
	".\INCLUDE\emul.h"\
	".\INCLUDE\sound.h"\
	".\INCLUDE\video.h"\
	".\ARCH\WIN32\vid-drv.h"\
	".\ARCH\WIN32\main.h"\
	{$(INCLUDE)}"\sys\Types.h"\
	".\ARCH\WIN32\config.h"\
	".\INCLUDE\cpu.h"\
	

"$(INTDIR)\Main.obj" : $(SOURCE) $(DEP_CPP_MAIN_) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=".\arch\win32\snd-drv.c"
DEP_CPP_SND_D=\
	".\INCLUDE\xgs.h"\
	".\INCLUDE\sound.h"\
	".\ARCH\WIN32\snd-drv.h"\
	".\ARCH\WIN32\config.h"\
	".\INCLUDE\cpu.h"\
	

"$(INTDIR)\snd-drv.obj" : $(SOURCE) $(DEP_CPP_SND_D) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=".\arch\win32\adb-drv.c"
DEP_CPP_ADB_D=\
	".\INCLUDE\xgs.h"\
	".\ARCH\WIN32\main.h"\
	".\INCLUDE\adb.h"\
	".\ARCH\WIN32\adb-drv.h"\
	".\INCLUDE\emul.h"\
	".\INCLUDE\video.h"\
	".\ARCH\WIN32\vid-drv.h"\
	".\ARCH\WIN32\config.h"\
	".\INCLUDE\cpu.h"\
	

"$(INTDIR)\adb-drv.obj" : $(SOURCE) $(DEP_CPP_ADB_D) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
# End Target
# End Project
################################################################################
