function ReceiveNewTxn(txn):
    masterInfo =∅
    foreach granule in txn.readSet⋃txn.writeSet
        masterInfo = masterInfo⋃LookupMaster.Lookup(granule)
    txn.masterInfo = masterInfo
    if(num of unique regions in txn.masterInfo == 1)
        txn.singleHome = true
        send txn to InsertIntoLocalLog(txn) of that home region
    else
        txn.singleHome = false
        send txn to the multi-home txn ordering module
        //ordering module orders all multi-home txns and calls
        //InsertIntoLocalLog(txn) ateveryregion in this order

functionInsertIntoLocalLog(txn):
    if(txn.singleHome == true)
        append txn at end of localLogBatch
    else
        accessSet =∅
        foreach <granule,granule.homeRegionID> in txn.masterInfo
            if(granule.homeRegionID == this.regionID)
                accessSet = accessSet ⋃ granule
        
        if(accessSet6=∅)
            t = new LockOnlyTxn(txn.ID,accessSet)
            append t at end of localLogBatch
    
    if(isComplete(localLogBatch))
        batchID = insert localLogBatch into Paxos-managed local log
        call ReceiveBatch(localLogBatch, batchID) at each region
        init new localLogBatch

functionReceiveBatch(batch, batchID):
    localLogs[batch.regionID][batchID] = batch
    prevID = largest batch ID in global log from batch.regionID
    while (localLogs[batch.regionID][prevID+1]6=null)
        append localLogs[batch.regionID][prevID+1] into global log
        prevID = prevID + 1
        
Lock manager thread code that continuously runs:
    txn = getNextTxnFromGlobalLog() //block until exists
    if(txn.isSingleHome == false and txn.lockOnlyTxn == false)
        ExecutionEngine.process(txn) //don’t request locks yet
    else 
        Call RequestLocks(txn) //pseudocode in Figure 6
        
Code called after all locks for single-home txn acquired:
    ExecutionEngine.process(txn)
    
Execution engine code called prior to read/write of granule:
    if(don’t have lock on granule) //can happen if txn is multi-home
        Block until have lock //wait for needed lockOnlyTxn
        //will always eventually get lock since SLOG is deadlock-free
        
    if(txn.masterInfo[granule]6=granule.header.masterInfo)
        RELEASE LOCKS AND ABORT TRANSACTION
        
Execution engine code run at end of every transaction:
    ReleaseLocks(txn)