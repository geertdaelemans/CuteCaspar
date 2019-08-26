TEMPLATE = subdirs

SUBDIRS += \
    Caspar \
    Common \
    Core \
    CuteCaspar \
    QMidi

Core.depends = Caspar Common
CuteCaspar.depends = Caspar Common Core
