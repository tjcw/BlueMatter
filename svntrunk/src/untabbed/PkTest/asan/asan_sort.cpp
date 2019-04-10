/* Copyright 2001, 2019 IBM Corporation
 *
 * Redistribution and use in source and binary forms, with or without modification, are permitted provided that the 
 * following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this list of conditions and the 
 * following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the 
 * following disclaimer in the documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, 
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE 
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, 
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR 
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, 
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE 
 * USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
 #include <stdio.h>
#include <string.h>
#include <sstream>

#include <asan_memory_server_bgltree.h>

#include <inmemdb/GlobalParallelObjectManager.hpp>
#include <inmemdb/NamedAllReduce.hpp>

NamedAllReduce<unsigned long long, NAMED_ALLREDUCE_SUM>   AllReduce;
#include <inmemdb/NamedBarrier.hpp>


#include <inmemdb/Table.hpp>

#include <inmemdb/types.hpp>
#include <inmemdb/utils.hpp>
#include <inmemdb/SortGen.hpp>
#include <inmemdb/IndySort.hpp>

#include <sstream>

#ifndef PKFXLOG_DEBUG_ASAN_SORT
#define PKFXLOG_DEBUG_ASAN_SORT ( 0 ) 
#endif

#ifndef PKFXLOG_RUN_SORT
#define PKFXLOG_RUN_SORT ( 0 )
#endif

#define TEST_RECORD_SIZE 100

int PkStartType = PkStartAllCores;

#if defined(PKTRACE)
int PkTracing = 1;
#else
int PkTracing = 0;
#endif

// This object is used by both cores
ASAN_MemoryServer asanMemoryServer;

struct DataGetRemainderOfRecordRequest
  {
    char      mDelimiter;
    int       mInode;
    long long mBlockIndex;
    char*     mRecvBuffer;
    int*      mRecordIndexAddress;
    int       mRecvBufferSizeFree;
  };

struct DataGetRemainderOfRecordResponse
  {
    char*     mRecvBuffer;
    pkFiberControlBlockT * FCB;
    int       mLength;
    int*      mRecordIndexAddress;
    char      mData[ 0 ];
  };

struct FlushMemoryAck
{
    pkFiberControlBlockT * FCB;
};

int FlushMemoryAckFx( void* arg )
  {
    FlushMemoryAck * Pkt = (FlushMemoryAck *) arg;
    
    pkFiberUnblock( Pkt->FCB );

    BegLogLine( PKFXLOG_DEBUG_ASAN_SORT )
      << "FlushMemoryAckFx:: "
      << " Got an ack for context: " << Pkt->FCB
      << EndLogLine;

    return 0;
  }

int FlushMemoryFx( void* arg )
  {
    rts_dcache_evict_normal();
    
    FlushMemoryAck* ResPkt = (FlushMemoryAck *)  PkActorReserveResponsePacket( FlushMemoryAckFx,
            sizeof( FlushMemoryAck ) );
    
    unsigned long SourceNodeId;
    unsigned long SourceCoreId;
    pkFiberControlBlockT* Context = NULL;
    PkActorGetPacketSourceContext( &SourceNodeId, &SourceCoreId, &Context );   
    
    ResPkt->FCB           = Context;
    
    PkActorPacketDispatch( ResPkt );

    BegLogLine( PKFXLOG_DEBUG_ASAN_SORT )
      << "FlushMemoryFx:: "
      << " Flushed memory, returning ack to: "
      << " SourceNodeId: " << SourceNodeId
      << " SourceCoreId: " << SourceCoreId
      << " Context: " << Context
      << EndLogLine;
    
    return 0;
  }

int ProcessDataGetRemoteRemaiderOfRecordResponseFx( void * arg )
{
  DataGetRemainderOfRecordResponse* Pkt = (DataGetRemainderOfRecordResponse *) arg;
 
  char* BufferPtr = Pkt->mRecvBuffer;
  int Length  = Pkt->mLength;
  
  memcpy( BufferPtr, Pkt->mData, Length );  
  
  int*  RecordIndexAddress      = Pkt->mRecordIndexAddress;
  *RecordIndexAddress += Length;
  
  pkFiberUnblock( Pkt->FCB );

  BegLogLine( PKFXLOG_DEBUG_ASAN_SORT )
    << "ProcessDataGetRemoteRemaiderOfRecordResponseFx:: Just called pkFiberUnblock" 
    << " Length: " << Length
    << " BufferPtr: " << (void *) BufferPtr
    << EndLogLine;
  
  return 0;
}

int ProcessDataGetRemoteRemaiderOfRecordRequestFx( void * arg )
{
  DataGetRemainderOfRecordRequest* Pkt = (DataGetRemainderOfRecordRequest *) arg;
  
  char Delimiter = Pkt->mDelimiter;
  int  Inode     = Pkt->mInode;
  long long BlockIndex     = Pkt->mBlockIndex;
  char* RecvBuffer         = Pkt->mRecvBuffer;
  int   RecvBufferSizeFree = Pkt->mRecvBufferSizeFree;
  int*  RecordIndexAddress      = Pkt->mRecordIndexAddress;
  
  unsigned long SourceNodeId;
  unsigned long SourceCoreId;
  pkFiberControlBlockT* Context = NULL;
  PkActorGetPacketSourceContext( &SourceNodeId, &SourceCoreId, &Context );   

  BegLogLine( PKFXLOG_DEBUG_ASAN_SORT )
    << "ProcessDataGetRemoteRemaiderOfRecordRequestFx:: "
    << " Inode: " << Inode
    << " Delimiter: " << (int) Delimiter
    << " BlockIndex: " << BlockIndex
    << " RecvBuffer: " << (void *) RecvBuffer
    << " RecvBufferSizeFree: " << (void *) RecvBufferSizeFree
    << " SourceNodeId: " << SourceNodeId
    << " SourceCoreId: " << SourceCoreId
    << EndLogLine;

  char* LocalBlockPtr = GetLocalDataPtr( Inode, BlockIndex, 0 );
  
  StrongAssertLogLine( LocalBlockPtr != NULL )
    << "ProcessDataGetRemoteRemaiderOfRecordRequestFx:: "
    << " ERROR:: LocalBlockPtr is NULL!"
    << " Inode: " << Inode
    << " BlockIndex: " << BlockIndex
    << EndLogLine;
  
  BegLogLine( PKFXLOG_DEBUG_ASAN_SORT )
    << "ProcessDataGetRemoteRemaiderOfRecordRequestFx:: "
    << " Inode: " << Inode
    << " Delimiter: " << (int) Delimiter
    << " BlockIndex: " << BlockIndex
    << " RecvBuffer: " << (void *) RecvBuffer
    << " RecvBufferSizeFree: " << (void *) RecvBufferSizeFree
    << " LocalBlockPtr: " << (void *) LocalBlockPtr
    << EndLogLine;

  int RemainderDataLength = 0;
  for( int c = 0; c < ASAN_MEMORY_BLOCK_SIZE; c++ )
    {
      char CurrentChar = LocalBlockPtr[ c ];
      
      if( CurrentChar == Delimiter )
  {
    RemainderDataLength = c + 1;
    // Send the last packet
    // ASSUMPTION: There's at most one packet worth to send back
    
    StrongAssertLogLine( RemainderDataLength <= RecvBufferSizeFree )
      << "ERROR:: RemainderDataLength: " << RemainderDataLength
      << " RecvBufferSizeFree: " << RecvBufferSizeFree
      <<  EndLogLine;
    
    int SizeToSend = sizeof( DataGetRemainderOfRecordResponse ) + RemainderDataLength;
    
    DataGetRemainderOfRecordResponse* ResPkt = (DataGetRemainderOfRecordResponse *)
      PkActorReserveResponsePacket( ProcessDataGetRemoteRemaiderOfRecordResponseFx,
            SizeToSend );
    
    ResPkt->mLength = RemainderDataLength;
    ResPkt->mRecvBuffer = RecvBuffer;
    ResPkt->mRecordIndexAddress = RecordIndexAddress;
    
    memcpy( ResPkt->mData, LocalBlockPtr, RemainderDataLength );
        
    ResPkt->FCB           = Context;
    
    BegLogLine( PKFXLOG_DEBUG_ASAN_SORT )
      << "ProcessDataGetRemoteRemaiderOfRecordRequestFx:: "
      << " SourceNodeId: " << SourceNodeId
      << " SourceCoreId: " << SourceCoreId
      << " Context: " << Context
      << " RemainderDataLength: " << RemainderDataLength
      << " SizeToSend: " << SizeToSend
      << " RecvBuffer: " << (void *) RecvBuffer
      << EndLogLine;
    
    PkActorPacketDispatch( ResPkt );

    break; 
  } 
    } 

  // Tick the IsFirstRecordTaken flag
  LocalDataAccessKey key;
  key.inode      = Inode;
  key.blockIndex = BlockIndex;
  
  LocalDataMap_T::iterator iter = LocalDataMap.find( key );

  StrongAssertLogLine( iter != LocalDataMap.end() )
    << "ERROR:: Valid mapping to a block is expected "
    << " Inode: " << Inode
    << " BlockIndex: " << BlockIndex
    << EndLogLine;
  
  iter->second.IsFirstRecordTaken = 1;

  BegLogLine( PKFXLOG_DEBUG_ASAN_SORT )
    << "ProcessDataGetRemoteRemaiderOfRecordRequestFx:: "
    << " Set iter->second.IsFirstRecordTaken to: "  << (int ) iter->second.IsFirstRecordTaken
    << " at addr: " << (void * ) &iter->second.IsFirstRecordTaken
    << EndLogLine;

  return 0; 
}

// NOTE: After return of the below function LocalRecordsIndex 
// Should be updated with the length of the remainder
void GetRemoteRemainderOfRecord( int DataOwner,
         int Inode,
         long long BlockIndex,
         char* LocalRecords,
         int*   LocalRecordsIndex,
         int   MaxLocalRecordsSize,
         char  Delimiter )
{
  DataGetRemainderOfRecordRequest* Pkt = (DataGetRemainderOfRecordRequest *)
    PkActorReservePacket( DataOwner,
        PkCoreGetId(),
        ProcessDataGetRemoteRemaiderOfRecordRequestFx,
        sizeof( DataGetRemainderOfRecordRequest ) );
  
  Pkt->mInode = Inode;
  Pkt->mDelimiter = Delimiter;
  Pkt->mBlockIndex = BlockIndex;
  Pkt->mRecvBuffer = & LocalRecords[ *LocalRecordsIndex ];
  Pkt->mRecvBufferSizeFree = MaxLocalRecordsSize - *LocalRecordsIndex;
  Pkt->mRecordIndexAddress = LocalRecordsIndex;
  
  BegLogLine( PKFXLOG_BGLTREE_DEBUG )
    << "GetRemoteRemainderOfRecord: About to dispatch a packet to "
    << " NodeId: " << DataOwner
    << " Inode: " << Inode
    << " BlockIndex: " << BlockIndex
    << EndLogLine;
  
  PkActorPacketDispatch( Pkt );
  
  PkFiberBlock();

}

int BlockDataToRecordData( int   inode, 
         int   sortFileSize,
         char  delimiter, 
         char* LocalRecords,
         int   MaxLocalRecordsSize )
  {
    BegLogLine( PKFXLOG_DEBUG_ASAN_SORT )
      << "BlockDataToRecordData: Entering..."
      << " inode: " << inode
      << " sortFileSize: " << sortFileSize
      << " delimiter: " << (int) delimiter
      << " MaxLocalRecordsSize: " << MaxLocalRecordsSize
      << " @LocalRecords: " << (void *) LocalRecords
      << EndLogLine;
    
    LocalBlocksForInodeManagerIF* LocalBlockManagerIF = asanMemoryServer.GetLocalBlocks( inode );
    
    int LocalRecordsIndex = 0;
    int LocalRecordsCount = 0;

    if( LocalBlockManagerIF != NULL )
      {	
  LocalBlockManagerIF->SortBlockList();
  BlockRep* LocalBlock = LocalBlockManagerIF->GetFirstBlock();

  // Figure out if this node has the very first record. 
  // In this case the first record starts at offset=0 and goes up to the
  // first delimiter
  
  // Seek up to first delimiter and start moving records out.
  // The node that owns the first byte of a record owns the record
  
  // On the last record in the block, if end of block is reached
  // and no delimiter is found then we fetch the rest of the record 
  // from the neighboring node
  
  int LocalRecordCount = 0;
  // char* StartOfRecord = LocalBlock->mBlockPtr;
  int StartOfRecord = -1;
  int CurrentRecordLength = 0;
    
  // Iterate over local blocks
  while( LocalBlock != NULL )
    {       	
      char* BlockPtr = LocalBlock->mBlockPtr;

      long long BlockIndex = LocalBlock->mBlockIndex;
      long long StartBlockOffsetInFile = BlockIndex * ASAN_MEMORY_BLOCK_SIZE;
      
      StrongAssertLogLine( BlockPtr != NULL )
        << "BlockDataToRecordData:: ERROR:: " 
        << " BlockPtr should not be NULL here"
        << " BlockIndex: " << LocalBlock->mBlockIndex
        << " inode: " << inode
        << " LocalBlock: " << (void *) LocalBlock
        << EndLogLine;	    

      BegLogLine( PKFXLOG_DEBUG_ASAN_SORT )
        << "BlockDataToRecordData:: Working on block ptr: " << (void * ) BlockPtr
        << " BlockIndex: " << LocalBlock->mBlockIndex
        << " inode: " << inode
        << EndLogLine;

      StartOfRecord = -1;
      CurrentRecordLength = 0;
      // Skip to the start of the second record
      for( int c = 0; c < ASAN_MEMORY_BLOCK_SIZE; c++ )
        {
    char CurrentChar = BlockPtr[ c ];
    if( CurrentChar == delimiter )
      {
        StartOfRecord = c+1;
        break;
      }
        
    StrongAssertLogLine(( c + StartBlockOffsetInFile ) < sortFileSize )
      << "BlockDataToRecordData:: ERROR:: End of file reached before start of second record found "
      << " sortFileSize: " << sortFileSize
      << " c: " << c
      << " StartBlockOffsetInFile: " << StartBlockOffsetInFile
      << EndLogLine;
        }

      BegLogLine( PKFXLOG_DEBUG_ASAN_SORT )
        << "BlockDataToRecordData: StartOfRecord: " << StartOfRecord
        << EndLogLine;
      
      // ASSUMPTION: This block contains a record boundary
      StrongAssertLogLine( StartOfRecord != -1 )
        << "ERROR:: ASSUMPTION: Each block contains a record boundary."
        << EndLogLine;
      
      int EndOfFile = 0;
      // Go down the block moving records
      
      #define RECORD_BUFFER_FOR_LOGGING_SIZE 1024
      char RecordBuffForLogging[ RECORD_BUFFER_FOR_LOGGING_SIZE ];
      bzero( RecordBuffForLogging, RECORD_BUFFER_FOR_LOGGING_SIZE );

      for( int c = StartOfRecord; c < ASAN_MEMORY_BLOCK_SIZE; c++ )
        {
    char CurrentChar = BlockPtr[ c ];
    CurrentRecordLength++;
    
    if( CurrentChar == delimiter )
      {	      
        // Got a record		    
        // Include the delimiter in the record
        memcpy( & LocalRecords[ LocalRecordsIndex ], 
          & BlockPtr[ StartOfRecord ],
          CurrentRecordLength );

        AssertLogLine( CurrentRecordLength < RECORD_BUFFER_FOR_LOGGING_SIZE )
          << "BlockDataToRecordData:: "
          << "ERROR:: CurrentRecordLength: " 
          << CurrentRecordLength 
          << " RECORD_BUFFER_FOR_LOGGING_SIZE: " 
          << RECORD_BUFFER_FOR_LOGGING_SIZE
          << EndLogLine;

        memcpy( RecordBuffForLogging, 
          &LocalRecords[ LocalRecordsIndex ],
          CurrentRecordLength );
        
        BegLogLine( PKFXLOG_DEBUG_ASAN_SORT )
          << "BlockDataToRecordData: "
          << " Moved record index: " << LocalRecordsIndex
          << " CurrentRecordLength: " << CurrentRecordLength
          << " MoveTarget: " << (void *) & LocalRecords[ LocalRecordsIndex ]
          << " MoveSource: " << (void *) & BlockPtr[ StartOfRecord ]
          << " LocalRecords[ " << LocalRecordsIndex << " ]: "
          << (char *) & LocalRecords[ LocalRecordsIndex ]
          << EndLogLine;
        
        LocalRecordsCount++;

        LocalRecordsIndex += CurrentRecordLength;
        
        StrongAssertLogLine( LocalRecordsIndex <= MaxLocalRecordsSize )
          << "ERROR:: LocalRecordsIndex: " << LocalRecordsIndex
          << " MaxLocalRecordsSize: " << MaxLocalRecordsSize
          << EndLogLine;
                
        StrongAssertLogLine( CurrentRecordLength == TEST_RECORD_SIZE )
          << "ERROR:: For the Indy sort the record length has to be: " 
          << TEST_RECORD_SIZE
          << " CurrentRecordLength: " << CurrentRecordLength
          << " LocalRecordsIndex: " << LocalRecordsIndex
          << EndLogLine;

        StartOfRecord = c + 1;
        CurrentRecordLength = 0;	      
      }
    
    if( ( c + StartBlockOffsetInFile ) >= sortFileSize )
      {
        // Reached the end of file. Time to break.
        BegLogLine( PKFXLOG_DEBUG_ASAN_SORT )
          << "BlockDataToRecordData:: "
          << " c: " << c
          << " StartBlockOffsetInFile: " << StartBlockOffsetInFile
          << " sortFileSize: " << sortFileSize
          << " BlockIndex: " << BlockIndex
          << " StartOfRecord: " << StartOfRecord
          << EndLogLine;

        EndOfFile = 1;
        break;		    
      }
        }
      
      // Last record is not complete, fetch the data from another node.
      if( ! EndOfFile && ( CurrentRecordLength > 0 ) )
        {
    long long NextBlockIndex = LocalBlock->mBlockIndex + 1;
    
    int DataOwner = GetDataOwner( inode, NextBlockIndex );
    
    BegLogLine( PKFXLOG_DEBUG_ASAN_SORT )
      << "BlockDataToRecordData: Getting the rest of the record from node: "
      << " DataOwner: " << DataOwner
      << " NextBlockIndex: " << NextBlockIndex
      << " CurrentRecordLength: " << CurrentRecordLength
      << EndLogLine;
    
    int StartingLocalRecordIndex = LocalRecordsIndex;

    memcpy( & LocalRecords[ LocalRecordsIndex ],
      & BlockPtr[ StartOfRecord ],
      CurrentRecordLength );
    
    LocalRecordsIndex += CurrentRecordLength;
    
    GetRemoteRemainderOfRecord( DataOwner, inode, 
              NextBlockIndex, LocalRecords,
              &LocalRecordsIndex,
              MaxLocalRecordsSize,
              delimiter );
    LocalRecordsCount++;

    // Indy check: Check to see if the incomming record is TEST_RECORD_SIZE bytes.
    int IncommingRecordSize = 0;		
    int CurrentTestIndex = StartingLocalRecordIndex;
    while( 1 )
      {
        char CurrentChar = LocalRecords[ CurrentTestIndex ];
        IncommingRecordSize++;
        CurrentTestIndex++;
        
        if( CurrentChar == delimiter )
          {
      StrongAssertLogLine( IncommingRecordSize == TEST_RECORD_SIZE )
        << "ERROR:: Incoming record not correct: " 
        << " IncommingRecordSize: " << IncommingRecordSize
        << " StartingLocalRecordIndex: " << StartingLocalRecordIndex
        << EndLogLine;

      break;
          }
        
        StrongAssertLogLine( IncommingRecordSize < TEST_RECORD_SIZE )
          << "ERROR:: "
          << "ERROR:: Incoming record not correct: " 
          << " IncommingRecordSize: " << IncommingRecordSize
          << " StartingLocalRecordIndex: " << StartingLocalRecordIndex		      
          << EndLogLine;		    
      }
        }
      
      LocalBlock = LocalBlockManagerIF->GetNextBlock();
      
      if( EndOfFile )
        StrongAssertLogLine( LocalBlock == NULL )
    << "BlockDataToRecordData:: Error:: End of file encountered, "
    << "but there are more records to process. "		
    << " LocalBlock: " << (void * ) LocalBlock
    << EndLogLine;
    }
  
  BegLogLine( PKFXLOG_DEBUG_ASAN_SORT )
    << "BlockDataToRecordData: "
    << "Waiting for the rest of the nodes to finish receiving it's records"
    << EndLogLine;
      }
    
    // Wait for every node to be finished with receiving it's data
    // PkCo_Barrier();  
    unsigned long long  TmpBuffer[ 1 ];
    TmpBuffer[ 0 ] = 0;
    AllReduce.Execute( TmpBuffer, 1 );
        
    if( LocalBlockManagerIF != NULL )
      {
  BlockRep* LocalBlock = LocalBlockManagerIF->GetFirstBlock();

  int RecordBufferIndex = 0;

  while( LocalBlock != NULL )
    {
      LocalDataAccessKey key;
      key.inode = inode;
      key.blockIndex = LocalBlock->mBlockIndex;
      
      BegLogLine( PKFXLOG_DEBUG_ASAN_SORT )
        << "BlockDataToRecordData:: Find a block in LocalDataMap for: "
        << " key.inode: " << key.inode
        << " key.blockIndex: " << key.blockIndex
        << EndLogLine;

      LocalDataMap_T::iterator iter = LocalDataMap.find( key );
      
      StrongAssertLogLine( iter != LocalDataMap.end() )
        << "ERROR:: Expected to find a local data block record for "
        << " inode: " << inode
        << " blockIndex: " << key.blockIndex
        << EndLogLine;
      
      BegLogLine( PKFXLOG_DEBUG_ASAN_SORT )
        << "BlockDataToRecordData: "
        << " Check if the first record of the block belongs to this node"
        << " iter->second.IsFirstRecordTaken: " << (int) iter->second.IsFirstRecordTaken
        << " at address: " << (void *) & iter->second.IsFirstRecordTaken
        << EndLogLine;
      
      if( !iter->second.IsFirstRecordTaken )	      
        {
    // First record of this block is complete and belongs on this node
    StrongAssertLogLine( iter->second.DataBlock == LocalBlock->mBlockPtr )
      << "ERROR:: Expect the block pointer in the LocalBlockManagerIF"
      << " and LocalDataMap to be the same" 
      << EndLogLine;

    // Move *only* the first record out.
    char* FirstBlockPtr = iter->second.DataBlock;
    int CurrentRecordLength = 0;
    for( int c = 0; c < ASAN_MEMORY_BLOCK_SIZE; c++ )
      {
        char CurrentChar = FirstBlockPtr[ c ];
        CurrentRecordLength++;
        if( CurrentChar == delimiter )
          {
      memcpy( & LocalRecords[ LocalRecordsIndex ], 
        FirstBlockPtr,
        CurrentRecordLength );
      
      BegLogLine( PKFXLOG_DEBUG_ASAN_SORT )
        << "BlockDataToRecordData: "
        << " Moved record index: " << LocalRecordsIndex
        << " CurrentRecordLength: " << CurrentRecordLength
        << " MoveTarget: " << (void *) & LocalRecords[ LocalRecordsIndex ]
        << " MoveSource: " << (void *) FirstBlockPtr
        << EndLogLine;			

      LocalRecordsIndex += CurrentRecordLength;
      LocalRecordsCount++;

      StrongAssertLogLine( CurrentRecordLength == TEST_RECORD_SIZE )
        << "ERROR:: For the Indy sort the record length has to be : "
        << TEST_RECORD_SIZE
        << " CurrentRecordLength: " << CurrentRecordLength
        << " LocalRecordsIndex: " << LocalRecordsIndex
        << EndLogLine;

      break;
          }
      }
        }

      LocalBlock = LocalBlockManagerIF->GetNextBlock();
    }
    
  BegLogLine( PKFXLOG_DEBUG_ASAN_SORT )
    << "BlockDataToRecordData: Total size of local records: " << LocalRecordsIndex
    << EndLogLine;
  
  char RecordBuff[ 1024 ];
  bzero( RecordBuff, 1024 );
  int RecordIndex = 0;
  int StartOfRecord = 0;
  int RecordSize    = 0;
  int RecordCount   = 0;
  while( RecordIndex < LocalRecordsIndex )
    {
      RecordSize++;
      if( LocalRecords[ RecordIndex ] == delimiter )
        {
    StrongAssertLogLine( RecordSize == TEST_RECORD_SIZE )
      << "ERROR:: Wrong record size for record at:  "
      << " RecordSize: " << RecordSize		  
      << " StartOfRecord: " << StartOfRecord	  
      << EndLogLine;
    
    memcpy( RecordBuff, 
      & LocalRecords[ StartOfRecord ],
      RecordSize );
    
    RecordCount++;
    
    BegLogLine( PKFXLOG_DEBUG_ASAN_SORT )
      << "BlockDataToRecordData: "
      << " LocalRecord[ " << ( RecordCount - 1 )<< " ]: " << RecordBuff
      << EndLogLine;
    
    RecordSize = 0;
    StartOfRecord = RecordIndex + 1;
        }
      
      RecordIndex++;	
    }
  
  StrongAssertLogLine( RecordCount == LocalRecordsCount )
    << "ERROR: RecordCount: " << RecordCount
    << " LocalRecordsCount: " << LocalRecordsCount
    << EndLogLine;

  BegLogLine( PKFXLOG_DEBUG_ASAN_SORT )
    << "BlockDataToRecordData: Total record count: " << RecordCount
    << EndLogLine;
      }    

    BegLogLine( PKFXLOG_DEBUG_ASAN_SORT )
      << "BlockDataToRecordData: Leaving..."
      << EndLogLine;
    
    return LocalRecordsCount;
  }

void FlushCore1()
{
  BegLogLine( PKFXLOG_DEBUG_ASAN_SORT )
    << "FlushCore1:: Entering... "
    << EndLogLine;

  int* Pkt = (int *)
    PkActorReservePacket( PkNodeGetId(),
        1,
        FlushMemoryFx,
        sizeof( int ) );
  
  PkActorPacketDispatch( Pkt );
  
  PkFiberBlock();

  BegLogLine( PKFXLOG_DEBUG_ASAN_SORT )
    << "FlushCore1:: Leaving... "
    << EndLogLine;
}

typedef inmemdb::Table< SortTupleInfo > TestTableType;
typedef TestTableType::Record StoredRecord;

void Run_Sort_Fiber( void * arg )
{
  // Wait to hear a signal from the host node 

  BegLogLine( PKFXLOG_DEBUG_ASAN_SORT )
    << "Entering Run_Sort()"
    << EndLogLine;

  unsigned SortFileInode = 0;
  unsigned long long SortFileSize = 0;
  
  if( PkNodeGetId() == 0 )
    {
      BGLPersonality Personality;
      rts_get_personality( &Personality, sizeof( BGLPersonality ) );
      
      char CommandFilePathName[ 256 ];
      char * BlockIdName = Personality.blockID;
      // sprintf( CommandFilePathName, "/ActiveSAN000/%s/public/Command.txt", BlockIdName );
      sprintf( CommandFilePathName, "/tmp/Command.txt", BlockIdName );
  
      BegLogLine( PKFXLOG_DEBUG_ASAN_SORT )
  << "Run_Sort: "
  << " CommandFilePathName: " << CommandFilePathName
  << EndLogLine;
      

      int fp = -1;
      
      double LastOpen = 0.0;
      do
  {
    double CurrentTime = PkTimeGetSecs();
    
    if( CurrentTime - LastOpen  > 1.0 )
      {
        fp = open( CommandFilePathName, O_RDONLY );
        
        if( fp == -1 )
    {
      if ( errno == ENOENT )
        {
          LastOpen = CurrentTime;
          // sleep( 1 );
          PkFiberYield();
        }
      else
        {
          StrongAssertLogLine( 0 )
      << "Failed to open filename: " 
      << CommandFilePathName
      << EndLogLine;
        }
    }
        else
    break;
      }
    else
      {
        PkFiberYield();
      }
  }
      while( 1 );

      BegLogLine( PKFXLOG_DEBUG_ASAN_SORT )
  << "Opened the command file: " << CommandFilePathName
  << EndLogLine;
      
      // Command file found in the asan file system
      // But not all the data might be visible yet.
      // fsync( fp );

      int CommandFileSize = 0;
      do
  {
    struct stat statbuf_commandfile;
    fstat( fp, &statbuf_commandfile );
    CommandFileSize = statbuf_commandfile.st_size;
    
    BegLogLine( PKFXLOG_RUN_SORT )
      << "CommandFileSize: " << CommandFileSize
      << EndLogLine;
    
    if( CommandFileSize == 0 )
      {
        // sleep( 1 );
      PkFiberYield();
      }	  
  } 
      while( CommandFileSize == 0 );
      
      char * CommandFileBuffer = (char *) PkHeapAllocate ( CommandFileSize );
      StrongAssertLogLine( CommandFileBuffer != NULL )
  << "ERROR: Allocating memory for command file buffer "
  << EndLogLine;      
      
      bzero( CommandFileBuffer, CommandFileSize );
      
      int SizeRead = 0;
      while( SizeRead < CommandFileSize )
  {
    int readLength = read( fp, 
         &CommandFileBuffer[ SizeRead ], 
         CommandFileSize - SizeRead );
    
    StrongAssertLogLine( readLength >= 0 )
      << "Failed to read the command file: " << CommandFilePathName
      << EndLogLine;

    SizeRead += readLength;
  }
            
      char delim[] = " ";
      char* CommandFileBufferPtr = CommandFileBuffer;
      char* CommandName = strsep( &CommandFileBufferPtr, delim );
      
      char* SortFilename = NULL;
      if( strcmp( CommandName, "sort" )  == 0 )
  {
    SortFilename = strsep( &CommandFileBufferPtr, delim );	  

    BegLogLine( PKFXLOG_DEBUG_ASAN_SORT )
      << "Doing a sort on filename: " << SortFilename
      << EndLogLine;
  }
      else if( strcmp( CommandName, "grep" )  == 0 )
  {
    SortFilename = strsep( &CommandFileBufferPtr, delim );	  

    BegLogLine( PKFXLOG_RUN_SORT )
      << "Doing a sort on filename: " << SortFilename
      << EndLogLine;
  }
  
      StrongAssertLogLine( SortFilename != NULL )
  << "ERROR:: Sort file name is not found"
  << EndLogLine;      

      int sortfile_fd = -1;
      do 
  {
    sortfile_fd = open( SortFilename, O_RDONLY );

    if( sortfile_fd <= 0 )
      {
        BegLogLine( 0 )
    << "Run_Sort:: Looking to open file: "
    << SortFilename
    << EndLogLine;

        // sleep( 1 );
        PkFiberYield();
      }
    else
      break;
  }
      while( sortfile_fd <= 0 );

      StrongAssertLogLine( sortfile_fd > 0 )
  << "ERROR:: Could not open sortfile: " << SortFilename
  << " errno: " << errno
  << EndLogLine;
      
      //      fsync( sortfile_fd );

      struct stat64 statbuf;
      fstat64( sortfile_fd, &statbuf );
      SortFileInode = statbuf.st_ino;
      SortFileSize  = statbuf.st_size;
      
      BegLogLine( PKFXLOG_DEBUG_ASAN_SORT )
  << "Run_Sort():: Before the all reduce"
  << " SortFileInode: " << SortFileInode
  << " SortFileSize: " << SortFileSize
  << EndLogLine;
      
      PkHeapFree( CommandFileBuffer );
      CommandFileBuffer = NULL;
    }
  
#define SORT_FILE_INFO_BUFFER_SIZE ( 2 )
  unsigned long long SortFileInfoBuffer[ SORT_FILE_INFO_BUFFER_SIZE ];

  SortFileInfoBuffer[ 0 ] = SortFileInode;
  SortFileInfoBuffer[ 1 ] = SortFileSize;
  
  BegLogLine( PKFXLOG_RUN_SORT )
    << "Run_Sort():: Before the all reduce of "
    << " SortFileInode: " << SortFileInode
    << " SortFileSize: " << SortFileSize
    << EndLogLine;  

  AllReduce.Execute( SortFileInfoBuffer, SORT_FILE_INFO_BUFFER_SIZE );
  
  // Every node has the inode for the file to sort
  SortFileInode = SortFileInfoBuffer[ 0 ];
  SortFileSize  = SortFileInfoBuffer[ 1 ];

  BegLogLine( PKFXLOG_DEBUG_ASAN_SORT )
    << "Run_Sort():: After the all reduce of "
    << " SortFileInode: " << SortFileInode
    << " SortFileSize: " << SortFileSize
    << EndLogLine;

  // IMPORTANT!!! Get the other core to flush it's memory
  // FlushCore1();

  char delimiter = '\n';

  int  LocalBlockCount = asanMemoryServer.GetLocalBlocksCount( SortFileInode );  

  BegLogLine( PKFXLOG_DEBUG_ASAN_SORT )
    << "Run_Sort():: "
    << " LocalBlockCount: " << LocalBlockCount
    << EndLogLine;

  char* Records = NULL;
  int  RecordsSize = 0;
  if( LocalBlockCount > 0 )
    {
      RecordsSize = sizeof( char ) * ASAN_MEMORY_BLOCK_SIZE * ( 2 * LocalBlockCount );
      
      BegLogLine( PKFXLOG_DEBUG_ASAN_SORT )
  << "Run_Sort():: "
  << " LocalBlockCount: " << LocalBlockCount
  << " RecordsSize: " << RecordsSize
  << EndLogLine;
      
      Records = (char *) PkHeapAllocate( RecordsSize );
      
      StrongAssertLogLine( Records != NULL )
  << "ERROR: Not enough memory for records allocation "
  << EndLogLine;
    }

  int LocalRecordCount = BlockDataToRecordData( SortFileInode, SortFileSize, delimiter, Records, RecordsSize );
    
  int max_mem = 100;
    
  // The records are ready.
  int MAX_MEMORY_FOR_SORT_PER_NODE = max_mem * 1024 * 1024;
  int MAX_RECORDS_PER_NODE         = MAX_MEMORY_FOR_SORT_PER_NODE / sizeof( StoredRecord );

  int    BinExpansionFactor         = BIN_COUNT_FACTOR;
  double TargetRecordsPerNodeFactor = TARGET_RECORDS_PER_NODE_FACTOR;  

  BegLogLine( PKFXLOG_RUN_SORT )
    << "Run_Sort: After BlockDataToRecordData:: "
    << " LocalRecordCount: " << LocalRecordCount
    << " max_mem: " << max_mem
    << " MAX_MEMORY_FOR_SORT_PER_NODE: " << MAX_MEMORY_FOR_SORT_PER_NODE
    << " MAX_RECORDS_PER_NODE: " << MAX_RECORDS_PER_NODE
    << " BinExpansionFactor: " << BinExpansionFactor
    << " TargetRecordsPerNodeFactor: " << TargetRecordsPerNodeFactor
    << EndLogLine;
  
  TestTableType* SortT = TestTableType::instance<inmemdb::BGL_Alloc< TestTableType > >
    ( "SortContainer",
      PkNodeGetCount(),
      BinExpansionFactor,
      TargetRecordsPerNodeFactor,
      MAX_RECORDS_PER_NODE );

  record* LocalRecords = (record *) Records;

  for( int i = 0; i < LocalRecordCount; i++ )
    {
      SortKey Key( LocalRecords[ i ].sortkey,
       LocalRecords[ i ].recnum, 
       LocalRecords[ i ].txtfld );
      
      SortTuple Tuple;

      if( SortT->insert( Key, Tuple ) )
  {
    StrongAssertLogLine( 1 )
      << "ERROR:: Insert failed on " 
      << " Key: " << Key
      << " Tuple: " << Tuple
      << EndLogLine;
    
  }
    }

  BegLogLine( PKFXLOG_RUN_SORT )
    << "Run_Sort: Before Barrier:: "
    << EndLogLine;

  // Barrier
  unsigned long long TmpBuffer[ 1 ];
  TmpBuffer[ 0 ] = 0;
  AllReduce.Execute( TmpBuffer, 1 );
  
  double tstart = PkTimeGetSecs();  
  
  BegLogLine( PKFXLOG_RUN_SORT )
    << "Run_Sort: Before sort():: "
    << EndLogLine;

  SortT->sort();

  double tfinish = PkTimeGetSecs();

  // Check if the records are sorted.
  BegLogLine( PKFXLOG_RUN_SORT )
    << "Run_Sort: Before checkSort():: "
    << EndLogLine;

  long long TotalRows = SortFileSize / 100;
  SortT->checkSort( TotalRows );
  
  unsigned long long NumRowsOnThisNode = LocalRecordCount;
  unsigned long long SizePerNode = ( sizeof( record ) * NumRowsOnThisNode );
  
#define B1024B (1024)
  unsigned long long ONE_GB = B1024B * B1024B * B1024B;
  unsigned long long ONE_TB = B1024B * B1024B * B1024B * B1024B;

  unsigned long long TotalBytes = ((unsigned long long) SortFileSize )
    * B1024B * B1024B * B1024B * B1024B ;
  
  double TotalSizeGB = (1.0 * TotalBytes) / ONE_GB;
  double TotalSizeTB = (1.0 * TotalBytes) / ONE_TB;
  
  // unsigned long long TotalNumberOfRows = TotalBytes / sizeof( record );
  TotalRows = TotalBytes / sizeof( record );
  
  BegLogLine( 1 )
    << "asan_sort::PkMain:: Sorting: "
    << " Rows on this node: " << NumRowsOnThisNode
    << " Total Rows: " << TotalRows
    << " Size of row: " << sizeof( record )
    << " Size of stored row: " << sizeof( StoredRecord )
    << " Size per node: " << SizePerNode
    << " Size per node (MB): " << (1.0*SizePerNode) / (1024.0*1024.0)
    << " Total size (bytes) : " << TotalBytes
    << " Total size (GB) : " << TotalSizeGB
    << " Total size (TB) : " << TotalSizeTB
    << " MAX_MEMORY_FOR_SORT_PER_NODE (MB): " << max_mem
    << " MAX_RECORDS_PER_NODE: " << MAX_RECORDS_PER_NODE
    << " BinExpansionFactor: " << BinExpansionFactor
    << " Timing: " << (tfinish-tstart)
    << EndLogLine;
    
  SortT->Finalize();  
  
  return;
}

enum {
  k_CoreIdForMemoryServer = 1 
};
int PkMain(int argc, char** argv, char** envp)
{  
  int TraceDumpFreq = atoi( argv[ 0 ] );
  int CoreId = rts_get_processor_id();
  
  BegLogLine( 1 )
    << "TraceDumpFreq: " << TraceDumpFreq
    << " CoreId: " << CoreId
    << EndLogLine;

  if( CoreId == 0 )
    {
    GlobalParallelObject::Init();
    AllReduce.Init( "AAA", PkNodeGetCount() );
    asanMemoryServer.Init();
    }
  
  rts_dcache_evict_normal();
  PkIntraChipBarrier();

  if( CoreId == k_CoreIdForMemoryServer )
    {
       asanMemoryServer.Run( TraceDumpFreq );
    }
  else
    {
      Run_Sort_Fiber( NULL );
      while( 1 ) {
        PkFiberYield();
      }
    }
  
  

  return 0;
}


int PkMainTmp(int argc, char** argv, char** envp)
{  
  int TraceDumpFreq = atoi( argv[ 0 ] );
  int CoreId = rts_get_processor_id();
  
  BegLogLine( 1 )
    << "TraceDumpFreq: " << TraceDumpFreq
    << " CoreId: " << CoreId
    << EndLogLine;

  if( CoreId == 0 )
    {
    GlobalParallelObject::Init();
    AllReduce.Init( "AAA", PkNodeGetCount() );
    asanMemoryServer.Init();
    
    PkFiberCreate( 1024 * 1024, Run_Sort_Fiber, NULL );
    
    asanMemoryServer.Run( TraceDumpFreq );

    }
  else
    while( 1 );
  
  return 0;
}