#include "EffectsDelegate.h"

#include "MidiNotes.h"

#include <QComboBox>

EffectsDelegate::EffectsDelegate(QObject *parent)
    : QStyledItemDelegate(parent)
{

}

EffectsDelegate::~EffectsDelegate()
{

}

QWidget *EffectsDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    // Create the combobox and populate it
    QComboBox *cb = new QComboBox(parent);
    const int row = index.row();
    QList<note> notesList = MidiNotes::getInstance()->getNotes();
    foreach(auto i, notesList) {
        cb->addItem(i.name, i.pitch);
    }
    return cb;

}

void EffectsDelegate::setEditorData(QWidget *editor, const QModelIndex &index) const
{
    QComboBox *cb = qobject_cast<QComboBox *>(editor);
    Q_ASSERT(cb);
    // get the index of the text in the combobox that matches the current value of the item
    const QString currentText = index.siblingAtColumn(2).data(Qt::EditRole).toString();
    const int cbIndex = cb->findText(currentText);
    // if it is valid, adjust the combobox
    if (cbIndex >= 0)
        cb->setCurrentIndex(cbIndex);
}

void EffectsDelegate::setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const
{
    QComboBox *cb = qobject_cast<QComboBox *>(editor);
    Q_ASSERT(cb);
    model->setData(index, cb->currentText(), Qt::EditRole);
}


