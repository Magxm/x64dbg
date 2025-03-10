#include "ThreadView.h"
#include "Configuration.h"
#include "Bridge.h"
#include "StringUtil.h"
#include "LineEditDialog.h"
#include "DisassemblyPopup.h"

void ThreadView::contextMenuSlot(const QPoint & pos)
{
    if(!DbgIsDebugging())
        return;

    QMenu menu(this); //create context menu
    mMenuBuilder->build(&menu);
    menu.exec(mapToGlobal(pos)); //execute context menu
}

void ThreadView::gotoThreadEntrySlot()
{
    QString addrText = getCellContent(getInitialSelection(), ColEntry);
    DbgCmdExecDirect(QString("disasm " + addrText));
}

void ThreadView::setupContextMenu()
{
    mMenuBuilder = new MenuBuilder(this);

    mMenuBuilder->addAction(makeCommandAction(new QAction(DIcon("thread-switch"), tr("Switch Thread"), this), "switchthread $"));
    mMenuBuilder->addAction(makeCommandAction(new QAction(DIcon("thread-pause"), tr("Suspend Thread"), this), "suspendthread $"));
    mMenuBuilder->addAction(makeCommandAction(new QAction(DIcon("thread-resume"), tr("Resume Thread"), this), "resumethread $"));
    mMenuBuilder->addAction(makeCommandAction(new QAction(DIcon("thread-pause"), tr("Suspend All Threads"), this), "suspendallthreads"));
    mMenuBuilder->addAction(makeCommandAction(new QAction(DIcon("thread-resume"), tr("Resume All Threads"), this), "resumeallthreads"));
    mMenuBuilder->addAction(makeCommandAction(new QAction(DIcon("thread-kill"), tr("Kill Thread"), this), "killthread $"));

    mMenuBuilder->addSeparator();

    mMenuBuilder->addAction(makeAction(DIcon("thread-setname"), tr("Set Name"), SLOT(setNameSlot())));

    // Set priority
    QAction* mSetPriorityIdle = makeCommandAction(new QAction(DIcon("thread-priority-idle"), tr("Idle"), this), "setprioritythread $, Idle");
    QAction* mSetPriorityAboveNormal = makeCommandAction(new QAction(DIcon("thread-priority-above-normal"), tr("Above Normal"), this), "setprioritythread $, AboveNormal");
    QAction* mSetPriorityBelowNormal = makeCommandAction(new QAction(DIcon("thread-priority-below-normal"), tr("Below Normal"), this), "setprioritythread $, BelowNormal");
    QAction* mSetPriorityHighest = makeCommandAction(new QAction(DIcon("thread-priority-highest"), tr("Highest"), this), "setprioritythread $, Highest");
    QAction* mSetPriorityLowest = makeCommandAction(new QAction(DIcon("thread-priority-lowest"), tr("Lowest"), this), "setprioritythread $, Lowest");
    QAction* mSetPriorityNormal = makeCommandAction(new QAction(DIcon("thread-priority-normal"), tr("Normal"), this), "setprioritythread $, Normal");
    QAction* mSetPriorityTimeCritical = makeCommandAction(new QAction(DIcon("thread-priority-timecritical"), tr("Time Critical"), this), "setprioritythread $, TimeCritical");
    MenuBuilder* mSetPriority = new MenuBuilder(this, [this, mSetPriorityIdle, mSetPriorityAboveNormal, mSetPriorityBelowNormal,
                  mSetPriorityHighest, mSetPriorityLowest, mSetPriorityNormal, mSetPriorityTimeCritical](QMenu*)
    {
        QString priority = getCellContent(getInitialSelection(), 6);
        QAction* selectedaction = nullptr;
        if(priority == tr("Normal"))
            selectedaction = mSetPriorityNormal;
        else if(priority == tr("AboveNormal"))
            selectedaction = mSetPriorityAboveNormal;
        else if(priority == tr("TimeCritical"))
            selectedaction = mSetPriorityTimeCritical;
        else if(priority == tr("Idle"))
            selectedaction = mSetPriorityIdle;
        else if(priority == tr("BelowNormal"))
            selectedaction = mSetPriorityBelowNormal;
        else if(priority == tr("Highest"))
            selectedaction = mSetPriorityHighest;
        else if(priority == tr("Lowest"))
            selectedaction = mSetPriorityLowest;

        mSetPriorityAboveNormal->setCheckable(true);
        mSetPriorityAboveNormal->setChecked(selectedaction == mSetPriorityAboveNormal); // true if mSetPriorityAboveNormal is selected
        mSetPriorityBelowNormal->setCheckable(true);
        mSetPriorityBelowNormal->setChecked(selectedaction == mSetPriorityBelowNormal); // true if mSetPriorityBelowNormal is selected
        mSetPriorityHighest->setCheckable(true);
        mSetPriorityHighest->setChecked(selectedaction == mSetPriorityHighest); // true if mSetPriorityHighest is selected
        mSetPriorityIdle->setCheckable(true);
        mSetPriorityIdle->setChecked(selectedaction == mSetPriorityIdle); // true if mSetPriorityIdle is selected
        mSetPriorityLowest->setCheckable(true);
        mSetPriorityLowest->setChecked(selectedaction == mSetPriorityLowest); // true if mSetPriorityLowest is selected
        mSetPriorityNormal->setCheckable(true);
        mSetPriorityNormal->setChecked(selectedaction == mSetPriorityNormal); // true if mSetPriorityNormal is selected
        mSetPriorityTimeCritical->setCheckable(true);
        mSetPriorityTimeCritical->setChecked(selectedaction == mSetPriorityTimeCritical); // true if mSetPriorityTimeCritical is selected
        return true;
    });
    mSetPriority->addAction(mSetPriorityTimeCritical);
    mSetPriority->addAction(mSetPriorityHighest);
    mSetPriority->addAction(mSetPriorityAboveNormal);
    mSetPriority->addAction(mSetPriorityNormal);
    mSetPriority->addAction(mSetPriorityBelowNormal);
    mSetPriority->addAction(mSetPriorityLowest);
    mSetPriority->addAction(mSetPriorityIdle);
    mMenuBuilder->addMenu(makeMenu(DIcon("thread-setpriority_alt"), tr("Set Priority")), mSetPriority);
    mMenuBuilder->addAction(makeAction(DIcon("thread-entry"), tr("Go to Thread Entry"), SLOT(gotoThreadEntrySlot())), [this](QMenu * menu)
    {
        bool ok;
        ULONGLONG entry = getCellContent(getInitialSelection(), 2).toULongLong(&ok, 16);
        if(ok && DbgMemIsValidReadPtr(entry))
        {
            menu->addSeparator();
            return true;
        }
        else
            return false;
    });

    MenuBuilder* mCopyMenu = new MenuBuilder(this);
    setupCopyMenu(mCopyMenu);
    // Column count cannot be zero
    mMenuBuilder->addSeparator();
    mMenuBuilder->addMenu(makeMenu(DIcon("copy"), tr("&Copy")), mCopyMenu);
    mMenuBuilder->loadFromConfig();
}

/**
 * @brief ThreadView::makeCommandAction Make action to execute the command. $ will be replaced with thread id at runtime.
 * @param action The action to bind
 * @param command The command to execute when the action is triggered.
 * @return action
 */
QAction* ThreadView::makeCommandAction(QAction* action, const QString & command)
{
    action->setData(QVariant(command));
    connect(action, SIGNAL(triggered()), this, SLOT(execCommandSlot()));
    return action;
}

/**
 * @brief ThreadView::execCommandSlot execute command slot for menus. Only used by command that reference thread id.
 */
void ThreadView::execCommandSlot()
{
    QAction* action = qobject_cast<QAction*>(sender());
    if(action)
    {
        QString command = action->data().toString();
        if(command.contains('$'))
        {
            for(int i : getSelection())
            {
                QString specializedCommand = command;
                specializedCommand.replace(QChar('$'), ToHexString(getCellUserdata(i, 1))); // $ -> Thread Id
                DbgCmdExec(specializedCommand);
            }
        }
        else
            DbgCmdExec(command);
    }
}

ThreadView::ThreadView(StdTable* parent) : StdTable(parent)
{
    enableMultiSelection(true);
    int charwidth = getCharWidth();
    addColumnAt(8 + charwidth * sizeof(unsigned int) * 2, tr("Number"), true, "", SortBy::AsInt);
    addColumnAt(8 + charwidth * sizeof(unsigned int) * 2, tr("ID"), true, "", ConfigBool("Gui", "PidTidInHex") ? SortBy::AsHex : SortBy::AsInt);
    addColumnAt(8 + charwidth * sizeof(duint) * 2, tr("Entry"), true, "", SortBy::AsHex);
    addColumnAt(8 + charwidth * sizeof(duint) * 2, tr("TEB"), true, "", SortBy::AsHex);
    addColumnAt(8 + charwidth * sizeof(duint) * 2, ArchValue(tr("EIP"), tr("RIP")), true, "", SortBy::AsHex);
    addColumnAt(8 + charwidth * 14, tr("Suspend Count"), true, "", SortBy::AsInt);
    addColumnAt(8 + charwidth * 12, tr("Priority"), true);
    addColumnAt(8 + charwidth * 12, tr("Wait Reason"), true);
    addColumnAt(8 + charwidth * 10, tr("Last Error"), true, "", SortBy::AsHex);
    addColumnAt(8 + charwidth * 16, tr("User Time"), true);
    addColumnAt(8 + charwidth * 16, tr("Kernel Time"), true);
    addColumnAt(8 + charwidth * 16, tr("Creation Time"), true);
    addColumnAt(8 + charwidth * 10, tr("CPU Cycles"), true, "", SortBy::AsHex);
    addColumnAt(8, tr("Name"), true);
    loadColumnFromConfig("Thread");

    //setCopyMenuOnly(true);
    connect(Bridge::getBridge(), SIGNAL(selectionThreadsSet(const SELECTIONDATA*)), this, SLOT(selectionThreadsSet(const SELECTIONDATA*)));
    connect(Bridge::getBridge(), SIGNAL(selectionThreadsGet(SELECTIONDATA*)), this, SLOT(selectionThreadsGet(SELECTIONDATA*)));
    connect(Bridge::getBridge(), SIGNAL(updateThreads()), this, SLOT(updateThreadListSlot()));
    connect(this, SIGNAL(doubleClickedSignal()), this, SLOT(doubleClickedSlot()));
    connect(this, SIGNAL(contextMenuSignal(QPoint)), this, SLOT(contextMenuSlot(QPoint)));

    setupContextMenu();

    new DisassemblyPopup(this, Bridge::getArchitecture());
}

void ThreadView::selectionThreadsSet(const SELECTIONDATA* selection)
{
    auto selectedThreadId = selection->start;
    auto rowCount = getRowCount();
    for(duint row = 0; row < rowCount; row++)
    {
        auto threadId = getCellUserdata(row, ColThreadId);
        if(threadId == selectedThreadId)
        {
            setSingleSelection(row);
            Bridge::getBridge()->setResult(BridgeResult::SelectionSet, 1);
            return;
        }
    }

    Bridge::getBridge()->setResult(BridgeResult::SelectionSet, 0);
}

void ThreadView::selectionThreadsGet(SELECTIONDATA* selection)
{
    auto threadId = getCellUserdata(getInitialSelection(), ColThreadId);
    selection->start = selection->end = threadId;
    Bridge::getBridge()->setResult(BridgeResult::SelectionGet, 1);
}

void ThreadView::updateThreadListSlot()
{
    THREADLIST threadList;
    memset(&threadList, 0, sizeof(THREADLIST));
    DbgGetThreadList(&threadList);
    setRowCount(threadList.count);
    auto tidFormat = ConfigBool("Gui", "PidTidInHex") ? "%X" : "%d";
    for(int i = 0; i < threadList.count; i++)
    {
        if(!threadList.list[i].BasicInfo.ThreadNumber)
            setCellContent(i, ColNumber, tr("Main"));
        else
            setCellContent(i, ColNumber, ToDecString(threadList.list[i].BasicInfo.ThreadNumber));
        setCellContent(i, ColThreadId, QString().sprintf(tidFormat, threadList.list[i].BasicInfo.ThreadId));
        setCellUserdata(i, ColThreadId, threadList.list[i].BasicInfo.ThreadId);
        setCellContent(i, ColEntry, ToPtrString(threadList.list[i].BasicInfo.ThreadStartAddress));
        setCellContent(i, ColTeb, ToPtrString(threadList.list[i].BasicInfo.ThreadLocalBase));
        setCellContent(i, ColCip, ToPtrString(threadList.list[i].ThreadCip));
        setCellContent(i, ColSuspendCount, ToDecString(threadList.list[i].SuspendCount));
        QString priorityString;
        switch(threadList.list[i].Priority)
        {
        case _PriorityIdle:
            priorityString = tr("Idle");
            break;
        case _PriorityAboveNormal:
            priorityString = tr("AboveNormal");
            break;
        case _PriorityBelowNormal:
            priorityString = tr("BelowNormal");
            break;
        case _PriorityHighest:
            priorityString = tr("Highest");
            break;
        case _PriorityLowest:
            priorityString = tr("Lowest");
            break;
        case _PriorityNormal:
            priorityString = tr("Normal");
            break;
        case _PriorityTimeCritical:
            priorityString = tr("TimeCritical");
            break;
        default:
            priorityString = tr("Unknown");
            break;
        }
        setCellContent(i, ColPriority, priorityString);
        QString waitReasonString;
        switch(threadList.list[i].WaitReason)
        {
        case _Executive:
            waitReasonString = "Executive";
            break;
        case _FreePage:
            waitReasonString = "FreePage";
            break;
        case _PageIn:
            waitReasonString = "PageIn";
            break;
        case _PoolAllocation:
            waitReasonString = "PoolAllocation";
            break;
        case _DelayExecution:
            waitReasonString = "DelayExecution";
            break;
        case _Suspended:
            waitReasonString = "Suspended";
            break;
        case _UserRequest:
            waitReasonString = "UserRequest";
            break;
        case _WrExecutive:
            waitReasonString = "WrExecutive";
            break;
        case _WrFreePage:
            waitReasonString = "WrFreePage";
            break;
        case _WrPageIn:
            waitReasonString = "WrPageIn";
            break;
        case _WrPoolAllocation:
            waitReasonString = "WrPoolAllocation";
            break;
        case _WrDelayExecution:
            waitReasonString = "WrDelayExecution";
            break;
        case _WrSuspended:
            waitReasonString = "WrSuspended";
            break;
        case _WrUserRequest:
            waitReasonString = "WrUserRequest";
            break;
        case _WrEventPair:
            waitReasonString = "WrEventPair";
            break;
        case _WrQueue:
            waitReasonString = "WrQueue";
            break;
        case _WrLpcReceive:
            waitReasonString = "WrLpcReceive";
            break;
        case _WrLpcReply:
            waitReasonString = "WrLpcReply";
            break;
        case _WrVirtualMemory:
            waitReasonString = "WrVirtualMemory";
            break;
        case _WrPageOut:
            waitReasonString = "WrPageOut";
            break;
        case _WrRendezvous:
            waitReasonString = "WrRendezvous";
            break;
        case _Spare2:
            waitReasonString = "Spare2";
            break;
        case _Spare3:
            waitReasonString = "Spare3";
            break;
        case _Spare4:
            waitReasonString = "Spare4";
            break;
        case _Spare5:
            waitReasonString = "Spare5";
            break;
        case _WrCalloutStack:
            waitReasonString = "WrCalloutStack";
            break;
        case _WrKernel:
            waitReasonString = "WrKernel";
            break;
        case _WrResource:
            waitReasonString = "WrResource";
            break;
        case _WrPushLock:
            waitReasonString = "WrPushLock";
            break;
        case _WrMutex:
            waitReasonString = "WrMutex";
            break;
        case _WrQuantumEnd:
            waitReasonString = "WrQuantumEnd";
            break;
        case _WrDispatchInt:
            waitReasonString = "WrDispatchInt";
            break;
        case _WrPreempted:
            waitReasonString = "WrPreempted";
            break;
        case _WrYieldExecution:
            waitReasonString = "WrYieldExecution";
            break;
        case _WrFastMutex:
            waitReasonString = "WrFastMutex";
            break;
        case _WrGuardedMutex:
            waitReasonString = "WrGuardedMutex";
            break;
        case _WrRundown:
            waitReasonString = "WrRundown";
            break;
        default:
            waitReasonString = "Unknown";
            break;
        }
        setCellContent(i, ColWaitReason, waitReasonString);
        setCellContent(i, ColLastError, QString("%1").arg(threadList.list[i].LastError, sizeof(unsigned int) * 2, 16, QChar('0')).toUpper());
        setCellContent(i, ColUserTime, FILETIMEToTime(threadList.list[i].UserTime));
        setCellContent(i, ColKernelTime, FILETIMEToTime(threadList.list[i].KernelTime));
        setCellContent(i, ColCreationTime, FILETIMEToDate(threadList.list[i].CreationTime));
        setCellContent(i, ColCpuCycles, ToLongLongHexString(threadList.list[i].Cycles));
        setCellContent(i, ColThreadName, threadList.list[i].BasicInfo.threadName);
    }
    mCurrentThreadId = -1;
    if(threadList.count)
    {
        int currentThread = threadList.CurrentThread;
        if(currentThread >= 0 && currentThread < threadList.count)
            mCurrentThreadId = threadList.list[currentThread].BasicInfo.ThreadId;
        BridgeFree(threadList.list);
    }
    reloadData();
}

QString ThreadView::paintContent(QPainter* painter, duint row, duint col, int x, int y, int w, int h)
{
    QString ret = StdTable::paintContent(painter, row, col, x, y, w, h);
    duint threadId = getCellUserdata(row, ColThreadId);
    if(threadId == mCurrentThreadId && !col)
    {
        painter->fillRect(QRect(x, y, w, h), QBrush(ConfigColor("ThreadCurrentBackgroundColor")));
        painter->setPen(QPen(ConfigColor("ThreadCurrentColor"))); //white text
        painter->drawText(QRect(x + 4, y, w - 4, h), Qt::AlignVCenter | Qt::AlignLeft, ret);
        ret = "";
    }
    return ret;
}

void ThreadView::doubleClickedSlot()
{
    duint threadId = getCellUserdata(getInitialSelection(), ColThreadId);
    DbgCmdExecDirect("switchthread " + ToHexString(threadId));

    QString addrText = getCellContent(getInitialSelection(), ColCip);
    DbgCmdExec("disasm " + addrText);
}

void ThreadView::setNameSlot()
{
    duint threadId = getCellUserdata(getInitialSelection(), ColThreadId);
    QString threadName = getCellContent(getInitialSelection(), ColThreadName);
    if(!SimpleInputBox(this, tr("Thread name - %1").arg(threadId), threadName, threadName, QString()))
        return;

    DbgCmdExec(QString("setthreadname %1, \"%2\"").arg(ToHexString(threadId)).arg(DbgCmdEscape(threadName)));
}
