// Copyright (c) 2014-2017 Josh Blum
// SPDX-License-Identifier: BSL-1.0

#pragma once
#include <Pothos/Config.hpp>
#include <Pothos/Proxy/Proxy.hpp>
#include <Pothos/Proxy/Environment.hpp>
#include <Pothos/Exception.hpp>
#include <QJsonObject>
#include <QJsonArray>
#include <QObject>
#include <QPointer>
#include <QString>
#include <QStringList>
#include <memory>
#include <chrono>
#include <Poco/Logger.h>
#include <Poco/Optional.h>

class EnvironmentEval;
class ThreadPoolEval;
class GraphBlock;

/*!
 * Information about a block that is used for background evaluation.
 * This is everything important we need to know about a block,
 * but extracted so we can access it in a thread-safe manner.
 * We store the GraphBlock in a QPointer which can only be
 * looked at in the GUI thread context. Which we will use
 * to update the block after evaluation from the GUI thread.
 */
struct BlockInfo
{
    QPointer<GraphBlock> block;
    bool isGraphWidget;
    QString id;
    size_t uid;
    bool enabled;
    QString zone;
    std::map<QString, QString> properties;
    QStringList constantNames; //preserves order
    std::map<QString, QString> constants;
    std::map<QString, QJsonObject> paramDescs;
    QJsonObject desc;
};

//! values to pass back to the gui thread to update the block
struct BlockStatus
{
    QPointer<GraphBlock> block;
    QPointer<QWidget> widget;
    std::map<QString, QString> propertyTypeInfos;
    std::map<QString, QString> propertyErrorMsgs;
    QStringList blockErrorMsgs;
    Poco::Optional<QJsonArray> inPortDesc;
    Poco::Optional<QJsonArray> outPortDesc;
    QJsonObject overlayDesc;
    QByteArray overlayDescStr;
    std::chrono::high_resolution_clock::time_point overlayExpired;
};

class BlockEval : public QObject
{
    Q_OBJECT
public:

    BlockEval(void);

    ~BlockEval(void);

    /*!
     * Does this information probably refer to this block?
     * We say probably, because this is a guess without UID.
     * Its an info match when the object ID matches,
     * and the block description path is identical.
     */
    bool isInfoMatch(const BlockInfo &info) const;

    //! Does this eval represent a graph widget?
    bool isGraphWidget(void) const
    {
        return _newBlockInfo.isGraphWidget;
    }

    /*!
     * Is the block eval in a usable state?
     * Environment active, no errors, ports queried...
     */
    bool isReady(void) const;

    /*!
     * This block should not be connected, remove from topology.
     */
    bool shouldDisconnect(void) const;

    /*!
     * Check the current evaluated state to see if a port exists.
     * This is used by the topology evaluation logic to check
     * before making a connection.
     */
    bool portExists(const QString &name, const bool isInput) const;

    /*!
     * Get the remote proxy block from the evaluator
     */
    Pothos::Proxy getProxyBlock(void) const;

    /*!
     * Called under re-eval to apply the latest info.
     * This call should take the info and not process.
     */
    void acceptInfo(const BlockInfo &info);

    /*!
     * Called under re-eval to apply the latest environment.
     * This call should take the info and not process.
     */
    void acceptEnvironment(const std::shared_ptr<EnvironmentEval> &env);

    /*!
     * Called under re-eval to apply the latest thread pool.
     * This call should take the info and not process.
     */
    void acceptThreadPool(const std::shared_ptr<ThreadPoolEval> &tp);

    /*!
     * Perform update work after changes applied.
     */
    void update(void);

private slots:

    /*!
     * Update the block from the gui thread context.
     */
    void postStatusToBlock(const BlockStatus &status);

    /*!
     * Call block eval from the gui thread context.
     * This is required for blocks that have widgets.
     */
    bool blockEvalInGUIContext(void);

private:

    //! critical change? need to make a new block
    bool hasCriticalChange(void) const;

    //! any setters that changed so we can re-call them
    QStringList settersChangedList(void) const;

    //! detect a change in properties before vs after
    bool didPropKeyHaveChange(const QString &key) const;

    /*!
     * Detect a change in an expression
     * accounting for the constants dependency tree.
     */
    bool didExprHaveChange(const QString &expr) const;

    /*!
     * Is this constant used in any of the properties.
     * Use this logic to skip registering unused constants.
     */
    bool isConstantUsed(const QString &name) const;

    /*!
     * Determine all constants used by this expression
     * The depth parameter prevents infinite recursion.
     */
    QStringList getConstantsUsed(const QString &expr, const size_t depth = 0) const;

    /*!
     * Create the remote block evaluator if needed.
     * Call evalProperty on all properties.
     * Record error conditions of each property.
     * Record the data type of each property.
     * \return true for success, false for error
     */
    bool updateAllProperties(void);

    /*!
     * Apply constants to the evaluator.
     * \return true for success, false for error
     */
    bool applyConstants(void);

    /*!
     * The main evaluation procedure for dealing with changes.
     * Return true for success and false for failure.
     */
    bool evaluationProcedure(void);

    //! Internal helper for error message formatting
    void reportError(const QString &action, const Pothos::Exception &ex);

    //Tracking state for the eval environment:
    //Also stash the actual proxy environment here.
    //The proxy environment provided by eval may change,
    //which we will use to determine block re-evaluation.
    std::shared_ptr<EnvironmentEval> _newEnvironmentEval;
    std::shared_ptr<EnvironmentEval> _lastEnvironmentEval;
    Pothos::ProxyEnvironment::Sptr _newEnvironment;
    Pothos::ProxyEnvironment::Sptr _lastEnvironment;

    //Tracking state for the thread pool:
    //Also stash the actual thread pool here.
    //The proxy thread pool provided by eval may change,
    //which we will use to determine setting a new pool.
    std::shared_ptr<ThreadPoolEval> _newThreadPoolEval;
    std::shared_ptr<ThreadPoolEval> _lastThreadPoolEval;
    Pothos::Proxy _newThreadPool;
    Pothos::Proxy _lastThreadPool;

    BlockInfo _newBlockInfo;
    BlockInfo _lastBlockInfo;

    //! tracking status in the eval thread context
    BlockStatus _lastBlockStatus;

    //remote block evaluator
    Pothos::Proxy _blockEval;
    Pothos::Proxy _proxyBlock;
    bool _queryPortDesc;

    Poco::Logger &_logger;
};
