#ifndef EFFECTSDELEGATE_H
#define EFFECTSDELEGATE_H

#include <QStyledItemDelegate>

class EffectsDelegate : public QStyledItemDelegate
{
    Q_OBJECT

public:
    EffectsDelegate(QObject *parent = nullptr);
    ~EffectsDelegate() override;

    QWidget * createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
    void setEditorData(QWidget *editor, const QModelIndex &index) const override;
    void setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const override;
 };

#endif // EFFECTSDELEGATE_H
