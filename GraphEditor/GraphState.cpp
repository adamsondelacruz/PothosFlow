// Copyright (c) 2013-2016 Josh Blum
// SPDX-License-Identifier: BSL-1.0

#include "MainWindow/IconUtils.hpp"
#include "GraphEditor/GraphState.hpp"
#include <QListWidgetItem>
#include <QLabel>
#include <iostream>

GraphState::GraphState(void)
{
    return;
}

GraphState::GraphState(const QString &iconName, const QString &description, const QByteArray &dump):
    iconName(iconName),
    description(description),
    dump(dump)
{
    return;
}

GraphState::GraphState(const QString &iconName, const QString &description, const QVariant &extraInfo):
    iconName(iconName),
    description(description),
    extraInfo(extraInfo)
{
    return;
}

GraphStateManager::GraphStateManager(QWidget *parent):
    QListWidget(parent)
{
    connect(this, &GraphStateManager::itemDoubleClicked, this, &GraphStateManager::handleItemDoubleClicked);
}

GraphStateManager::~GraphStateManager(void)
{
    return;
}


void GraphStateManager::handleItemDoubleClicked(QListWidgetItem *item)
{
    emit this->newStateSelected(_itemToIndex[item]);
}

void GraphStateManager::change(void)
{
    this->clear();
    _itemToIndex.clear();
    for (int i = this->numStates()-1; i >= 0; i--)
    {
        const auto &state = this->getStateAt(i);
        auto item = new QListWidgetItem(this);
        item->setIcon(makeIconFromTheme(state.iconName));
        this->addItem(item);
        auto desc = state.description.toHtmlEscaped();
        if (size_t(i) == this->getCurrentIndex()) desc = QString("<b>%1</b>").arg(desc);
        if (size_t(i) == this->getSavedIndex()) desc = QString("<i>%1</i>").arg(desc);
        this->setItemWidget(item, new QLabel(QString("<span>%1</span>").arg(desc)));
        _itemToIndex[item] = i;
    }
}
