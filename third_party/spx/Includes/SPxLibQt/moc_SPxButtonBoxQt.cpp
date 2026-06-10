/****************************************************************************
** Meta object code from reading C++ file 'SPxButtonBoxQt.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.12.12)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "SPxButtonBoxQt.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'SPxButtonBoxQt.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.12.12. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_SPxButtonBoxQt_t {
    QByteArrayData data[3];
    char stringdata0[26];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_SPxButtonBoxQt_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_SPxButtonBoxQt_t qt_meta_stringdata_SPxButtonBoxQt = {
    {
QT_MOC_LITERAL(0, 0, 14), // "SPxButtonBoxQt"
QT_MOC_LITERAL(1, 15, 9), // "TimerSlot"
QT_MOC_LITERAL(2, 25, 0) // ""

    },
    "SPxButtonBoxQt\0TimerSlot\0"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_SPxButtonBoxQt[] = {

 // content:
       8,       // revision
       0,       // classname
       0,    0, // classinfo
       1,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

 // slots: name, argc, parameters, tag, flags
       1,    0,   19,    2, 0x08 /* Private */,

 // slots: parameters
    QMetaType::Void,

       0        // eod
};

void SPxButtonBoxQt::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<SPxButtonBoxQt *>(_o);
        Q_UNUSED(_t)
        switch (_id) {
        case 0: _t->TimerSlot(); break;
        default: ;
        }
    }
    Q_UNUSED(_a);
}

QT_INIT_METAOBJECT const QMetaObject SPxButtonBoxQt::staticMetaObject = { {
    &QWidget::staticMetaObject,
    qt_meta_stringdata_SPxButtonBoxQt.data,
    qt_meta_data_SPxButtonBoxQt,
    qt_static_metacall,
    nullptr,
    nullptr
} };


const QMetaObject *SPxButtonBoxQt::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *SPxButtonBoxQt::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_SPxButtonBoxQt.stringdata0))
        return static_cast<void*>(this);
    if (!strcmp(_clname, "SPxButtonBox"))
        return static_cast< SPxButtonBox*>(this);
    return QWidget::qt_metacast(_clname);
}

int SPxButtonBoxQt::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QWidget::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 1)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 1;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 1)
            *reinterpret_cast<int*>(_a[0]) = -1;
        _id -= 1;
    }
    return _id;
}
struct qt_meta_stringdata_SPxButtonBoxButtonQt_t {
    QByteArrayData data[1];
    char stringdata0[21];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_SPxButtonBoxButtonQt_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_SPxButtonBoxButtonQt_t qt_meta_stringdata_SPxButtonBoxButtonQt = {
    {
QT_MOC_LITERAL(0, 0, 20) // "SPxButtonBoxButtonQt"

    },
    "SPxButtonBoxButtonQt"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_SPxButtonBoxButtonQt[] = {

 // content:
       8,       // revision
       0,       // classname
       0,    0, // classinfo
       0,    0, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

       0        // eod
};

void SPxButtonBoxButtonQt::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    Q_UNUSED(_o);
    Q_UNUSED(_id);
    Q_UNUSED(_c);
    Q_UNUSED(_a);
}

QT_INIT_METAOBJECT const QMetaObject SPxButtonBoxButtonQt::staticMetaObject = { {
    &QPushButton::staticMetaObject,
    qt_meta_stringdata_SPxButtonBoxButtonQt.data,
    qt_meta_data_SPxButtonBoxButtonQt,
    qt_static_metacall,
    nullptr,
    nullptr
} };


const QMetaObject *SPxButtonBoxButtonQt::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *SPxButtonBoxButtonQt::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_SPxButtonBoxButtonQt.stringdata0))
        return static_cast<void*>(this);
    if (!strcmp(_clname, "SPxButtonBoxButton"))
        return static_cast< SPxButtonBoxButton*>(this);
    return QPushButton::qt_metacast(_clname);
}

int SPxButtonBoxButtonQt::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QPushButton::qt_metacall(_c, _id, _a);
    return _id;
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
