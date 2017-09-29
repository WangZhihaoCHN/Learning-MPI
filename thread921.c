/*
   (C) 2017 by Wang Zhihao.
       From BUAA.
       该程序首先在每个进程中创建多个线程，然后每个产生的线程复制MPI_COMM_WORLD以形成一个环，
    即每个线程发送消息到其下一个等级，并在同一复制的MPI_COMM_WORLD内从其先前的等级接收消息
    以形成环。建议通过MPICH和openMPI两个实现来运行程序。
*/

#include "mpi.h"
#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>

#define BUFLEN 512
#define NTIMES 100
#define MAX_THREADS 10

/*
    Concurrent send and recv by multiple threads on each process. 
*/
void *thd_sendrecv( void * );
void *thd_sendrecv( void *comm_ptr )
{
    MPI_Comm     comm;
    int         my_rank, num_procs, next, buffer_size, namelen, idx;
    char        buffer[BUFLEN], processor_name[MPI_MAX_PROCESSOR_NAME];
    MPI_Status  status;

    // 世界通信组
    comm = *(MPI_Comm *) comm_ptr;

    //获得进程数、当前进程id、当前处理器名
    MPI_Comm_size( comm, &num_procs );
    MPI_Comm_rank( comm, &my_rank );
    MPI_Get_processor_name( processor_name, &namelen );

    fprintf( stderr, "Process %d on %s\n", my_rank, processor_name );
    strcpy( buffer, "hello there" );
    buffer_size = strlen(buffer)+1;

    if ( my_rank == num_procs-1 )
        next = 0;
    else
        next = my_rank+1;

    for ( idx = 0; idx < NTIMES; idx++ ) {
        if (my_rank == 0) {
            MPI_Send(buffer, buffer_size, MPI_CHAR, next, 99, comm);
            MPI_Send(buffer, buffer_size, MPI_CHAR, MPI_PROC_NULL, 299, comm);
            MPI_Recv(buffer, BUFLEN, MPI_CHAR, MPI_ANY_SOURCE, 99,
                     comm, &status);
            // fprintf( stderr, "Process 0 send %s, and received %s\n", buffer, buffer );
        }
        else {
            MPI_Recv(buffer, BUFLEN, MPI_CHAR, MPI_ANY_SOURCE, 99,
                     comm, &status);
            MPI_Recv(buffer, BUFLEN, MPI_CHAR, MPI_PROC_NULL, 299,
                     comm, &status);
            MPI_Send(buffer, buffer_size, MPI_CHAR, next, 99, comm);
            // fprintf( stderr, "Process %d send %s, and received %s\n", my_rank, buffer, buffer );
        }
        /* MPI_Barrier(comm); */
    }

    pthread_exit( NULL );
    return 0;
}



int main( int argc,char *argv[] )
{
    MPI_Comm   comm[ MAX_THREADS ];
    pthread_t  thd_id[ MAX_THREADS ];
    int        my_rank, ii, provided;
    int        num_threads;

    /* int MPI_Init_thread(int *argc, char *((*argv)[]), int required, int *provided)
       初始化MPI，并初始化MPI线程环境。
       该初始化过程类似于MPI_Init，argc和argv是可选项。
       required用于描述线程支持的等级，有4种。
     */
    MPI_Init_thread( &argc, &argv, MPI_THREAD_MULTIPLE, &provided );
    // 如果线程支持级别不足，则报错并终止MPI执行环境
    if ( provided != MPI_THREAD_MULTIPLE ) {    
        printf( "Aborting, MPI_THREAD_MULTIPLE is needed...\n" );
        MPI_Abort( MPI_COMM_WORLD, 1 );
    }

    // 获得当前进程号
    MPI_Comm_rank( MPI_COMM_WORLD, &my_rank );

    if ( my_rank == 0 ) {
        // 参数只有一个线程数，当参数多了，则报错并停止MPI执行
        if (argc != 2) {
            printf( "Error: %s num_threads\n", argv[0] );
            MPI_Abort( MPI_COMM_WORLD, 1 );
        }
        // 将参数（字符串类型）转换成整数
        num_threads = atoi( argv[1] );
        // 向通信组内被标记为1的进程向组内所有其他进程发送消息
        MPI_Bcast( &num_threads, 1, MPI_INT, 0, MPI_COMM_WORLD );
    }
    else
        MPI_Bcast( &num_threads, 1, MPI_INT, 0, MPI_COMM_WORLD );

    // 调用该函数时进程处于等待状态，直到通信子中所有进程都调用了该函数后才继续执行
    MPI_Barrier( MPI_COMM_WORLD );

    for ( ii=0; ii < num_threads; ii++ ) {
        // 复制现有现有的通讯器与其所有缓存的信息
        MPI_Comm_dup( MPI_COMM_WORLD, &comm[ii] );
        pthread_create( &thd_id[ii], NULL, thd_sendrecv, (void *) &comm[ii] );
    }
        
    for ( ii=0; ii < num_threads; ii++ )
        pthread_join( thd_id[ii], NULL );

    MPI_Finalize();
    return 0;
}
