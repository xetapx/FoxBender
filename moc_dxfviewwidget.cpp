/****************************************************************************
** Meta object code from reading C++ file 'dxfviewwidget.h'
**
** Created by: The Qt Meta Object Compiler version 69 (Qt 6.11.1)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "src/ui/widgets/dxfviewwidget.h"
#include <QtCore/qmetatype.h>

#include <QtCore/qtmochelpers.h>

#include <memory>


#include <QtCore/qxptype_traits.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'dxfviewwidget.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 69
#error "This file was generated using the moc from 6.11.1. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

#ifndef Q_CONSTINIT
#define Q_CONSTINIT
#endif

QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
QT_WARNING_DISABLE_GCC("-Wuseless-cast")
namespace {
struct qt_meta_tag_ZN13DxfViewWidgetE_t {};
} // unnamed namespace

template <> constexpr inline auto DxfViewWidget::qt_create_metaobjectdata<qt_meta_tag_ZN13DxfViewWidgetE_t>()
{
    namespace QMC = QtMocConstants;
    QtMocHelpers::StringRefStorage qt_stringData {
        "DxfViewWidget",
        "entitySelected",
        "",
        "entityIndex",
        "entityClicked",
        "QPointF",
        "scenePos",
        "entityHovered",
        "pointerMoved",
        "cancelRequested",
        "documentEdited",
        "documentEditCommitted",
        "DxfDocument",
        "before",
        "after",
        "description"
    };

    QtMocHelpers::UintData qt_methods {
        // Signal 'entitySelected'
        QtMocHelpers::SignalData<void(int)>(1, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Int, 3 },
        }}),
        // Signal 'entityClicked'
        QtMocHelpers::SignalData<void(int, const QPointF &)>(4, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Int, 3 }, { 0x80000000 | 5, 6 },
        }}),
        // Signal 'entityHovered'
        QtMocHelpers::SignalData<void(int)>(7, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Int, 3 },
        }}),
        // Signal 'pointerMoved'
        QtMocHelpers::SignalData<void(const QPointF &)>(8, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 5, 6 },
        }}),
        // Signal 'cancelRequested'
        QtMocHelpers::SignalData<void()>(9, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'documentEdited'
        QtMocHelpers::SignalData<void()>(10, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'documentEditCommitted'
        QtMocHelpers::SignalData<void(const DxfDocument &, const DxfDocument &, const QString &)>(11, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 12, 13 }, { 0x80000000 | 12, 14 }, { QMetaType::QString, 15 },
        }}),
    };
    QtMocHelpers::UintData qt_properties {
    };
    QtMocHelpers::UintData qt_enums {
    };
    return QtMocHelpers::metaObjectData<DxfViewWidget, qt_meta_tag_ZN13DxfViewWidgetE_t>(QMC::MetaObjectFlag{}, qt_stringData,
            qt_methods, qt_properties, qt_enums);
}
Q_CONSTINIT const QMetaObject DxfViewWidget::staticMetaObject = { {
    QMetaObject::SuperData::link<QGraphicsView::staticMetaObject>(),
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN13DxfViewWidgetE_t>.stringdata,
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN13DxfViewWidgetE_t>.data,
    qt_static_metacall,
    nullptr,
    qt_staticMetaObjectRelocatingContent<qt_meta_tag_ZN13DxfViewWidgetE_t>.metaTypes,
    nullptr
} };

void DxfViewWidget::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    auto *_t = static_cast<DxfViewWidget *>(_o);
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: _t->entitySelected((*reinterpret_cast<std::add_pointer_t<int>>(_a[1]))); break;
        case 1: _t->entityClicked((*reinterpret_cast<std::add_pointer_t<int>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<QPointF>>(_a[2]))); break;
        case 2: _t->entityHovered((*reinterpret_cast<std::add_pointer_t<int>>(_a[1]))); break;
        case 3: _t->pointerMoved((*reinterpret_cast<std::add_pointer_t<QPointF>>(_a[1]))); break;
        case 4: _t->cancelRequested(); break;
        case 5: _t->documentEdited(); break;
        case 6: _t->documentEditCommitted((*reinterpret_cast<std::add_pointer_t<DxfDocument>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<DxfDocument>>(_a[2])),(*reinterpret_cast<std::add_pointer_t<QString>>(_a[3]))); break;
        default: ;
        }
    }
    if (_c == QMetaObject::IndexOfMethod) {
        if (QtMocHelpers::indexOfMethod<void (DxfViewWidget::*)(int )>(_a, &DxfViewWidget::entitySelected, 0))
            return;
        if (QtMocHelpers::indexOfMethod<void (DxfViewWidget::*)(int , const QPointF & )>(_a, &DxfViewWidget::entityClicked, 1))
            return;
        if (QtMocHelpers::indexOfMethod<void (DxfViewWidget::*)(int )>(_a, &DxfViewWidget::entityHovered, 2))
            return;
        if (QtMocHelpers::indexOfMethod<void (DxfViewWidget::*)(const QPointF & )>(_a, &DxfViewWidget::pointerMoved, 3))
            return;
        if (QtMocHelpers::indexOfMethod<void (DxfViewWidget::*)()>(_a, &DxfViewWidget::cancelRequested, 4))
            return;
        if (QtMocHelpers::indexOfMethod<void (DxfViewWidget::*)()>(_a, &DxfViewWidget::documentEdited, 5))
            return;
        if (QtMocHelpers::indexOfMethod<void (DxfViewWidget::*)(const DxfDocument & , const DxfDocument & , const QString & )>(_a, &DxfViewWidget::documentEditCommitted, 6))
            return;
    }
}

const QMetaObject *DxfViewWidget::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *DxfViewWidget::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_staticMetaObjectStaticContent<qt_meta_tag_ZN13DxfViewWidgetE_t>.strings))
        return static_cast<void*>(this);
    return QGraphicsView::qt_metacast(_clname);
}

int DxfViewWidget::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QGraphicsView::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 7)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 7;
    }
    if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 7)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 7;
    }
    return _id;
}

// SIGNAL 0
void DxfViewWidget::entitySelected(int _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 0, nullptr, _t1);
}

// SIGNAL 1
void DxfViewWidget::entityClicked(int _t1, const QPointF & _t2)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 1, nullptr, _t1, _t2);
}

// SIGNAL 2
void DxfViewWidget::entityHovered(int _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 2, nullptr, _t1);
}

// SIGNAL 3
void DxfViewWidget::pointerMoved(const QPointF & _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 3, nullptr, _t1);
}

// SIGNAL 4
void DxfViewWidget::cancelRequested()
{
    QMetaObject::activate(this, &staticMetaObject, 4, nullptr);
}

// SIGNAL 5
void DxfViewWidget::documentEdited()
{
    QMetaObject::activate(this, &staticMetaObject, 5, nullptr);
}

// SIGNAL 6
void DxfViewWidget::documentEditCommitted(const DxfDocument & _t1, const DxfDocument & _t2, const QString & _t3)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 6, nullptr, _t1, _t2, _t3);
}
QT_WARNING_POP
