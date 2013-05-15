#####################################################################
# DirectUI project file
#####################################################################

include(shared.pri)
TEMPLATE = subdirs

isEqual( IN_PWD, $${OUT_PWD} ) {
    IS_OUT_OF_SOURCE = 0
} else {
    IS_OUT_OF_SOURCE = 1
}


system(pkg-config --exists xdamage) {
    DEFINES += HAVE_XDAMAGE
}

M_BUILD_PARTS = plainqt demos

#process the projects
for(PROJECT, $$list($$lower($$unique(M_BUILD_PARTS)))) {
    isEqual(PROJECT, plainqt) {
       addSubDirs(style)
       addSubDirs(style/qstyleplugin)
    } else:isEqual(PROJECT, tests) {
       addSubDirs(tests)
       macx:SUBDIRS -= tests
       win32:SUBDIRS -= tests
    } else:isEqual(PROJECT, demos) {
       addSubDirs(demos)
    } else {
       message(Unknown PROJECT: $$PROJECT)
    }
}

