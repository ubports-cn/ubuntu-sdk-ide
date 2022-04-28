
LIBS+=-lyaml-cpp

SOURCES += \
    $$PWD/project/snapcraftproject.cpp \
    $$PWD/project/snapcraftprojectfile.cpp \
    $$PWD/project/snapcraftprojectnode.cpp \
    $$PWD/project/snapcraftprojectmanager.cpp \
    $$PWD/project/snapcraftbuildconfigurationfactory.cpp \
    $$PWD/project/snapcraftbuildconfiguration.cpp \
    $$PWD/project/snapcraftrsyncstep.cpp \
    $$PWD/project/snapcraftstep.cpp \
    $$PWD/project/snapcraftbuildstepfactory.cpp \
    $$PWD/settings/snapcraftkitinformation.cpp \
    $$PWD/project/snaprunconfiguration.cpp \
    $$PWD/project/snaprunconfigurationfactory.cpp

HEADERS += \
    $$PWD/project/snapcraftproject.h \
    $$PWD/project/snapcraftprojectfile.h \
    $$PWD/project/snapcraftprojectnode.h \
    $$PWD/project/snapcraftprojectmanager.h \
    $$PWD/project/snapcraftbuildconfigurationfactory.h \
    $$PWD/project/snapcraftbuildconfiguration.h \
    $$PWD/project/snapcraftrsyncstep.h \
    $$PWD/project/snapcraftstep.h \
    $$PWD/project/snapcraftbuildstepfactory.h \
    $$PWD/settings/snapcraftkitinformation.h \
    $$PWD/project/snaprunconfiguration.h \
    $$PWD/project/snaprunconfigurationfactory.h

