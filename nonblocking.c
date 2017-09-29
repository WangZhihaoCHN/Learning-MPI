/*

MPI_Isend MPI_Irecv
描述：
   例1和2展示了阻塞和非阻塞点对点通信之间的区别。
   例2：MPI_Send / MPI_Recv（阻塞）
   例1：MPI_Isend / MPI_Irecv（非阻塞）
   
              sendbuff   recvbuff       sendbuff   recvbuff
                                         
              ########   ########       ########   ########
              #      #   #      #       #      #   #      #
          0   #  AA  #   #      #       #  AA  #   #  EE  #
              #      #   #      #       #      #   #      #
              ########   ########       ########   ########
     T        #      #   #      #       #      #   #      #
          1   #  BB  #   #      #       #  BB  #   #  AA  #
     a        #      #   #      #       #      #   #      #
              ########   ########       ########   ########
     s        #      #   #      #       #      #   #      #
          2   #  CC  #   #      #       #  CC  #   #  BB  #
     k        #      #   #      #       #      #   #      #
              ########   ########       ########   ########
     s        #      #   #      #       #      #   #      #
          3   #  DD  #   #      #       #  DD  #   #  CC  #
              #      #   #      #       #      #   #      #
              ########   ########       ########   ########
              #      #   #      #       #      #   #      #
          4   #  EE  #   #      #       #  EE  #   #  DD  #
              #      #   #      #       #      #   #      #
              ########   ########       ########   ########
              
                     BEFORE                    AFTER                                         


   每个任务将一个随机数组成的向量（sendbuff）传送到下一个任务（taskid+1）。最后一个任务将其传送到任
   务0。因此，每个任务从前面的任务接收一个向量并将其放入recvbuff中。
   
   此示例显示MPI_Isend和MPI_Irecv更适合完成此工作。

   MPI_Isend和MPI_Irecv是非阻塞的，这意味着函数调用在通信完成之前返回。使用非阻塞通信时，死锁可以避
   免，但使用它们时必须采取其他预防措施。特别要确定在某一时间点，你的数据已经有效到达！然后，在完成每个
   发送和/或接收之前，你需要进行MPI_Wait调用。
   
   很明显，在本例中使用非阻塞调用时，任务之间的所有交换都将同时发生。
   
   在通信之前，任务0收集从每个任务发送的所有向量的总和，并将其打印出来。类似地，在通信之后，任务0收集每
   个任务接收到的向量的所有和，并将其与通信时间一起打印出来。
   
   例5显示了如何使用阻塞通信（MPI_Send和MPI_Recv）来更有效地完成相同的工作。
   
   vecteur（buffsize）的大小作为运行时程序的参数给出。

*/


#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include "math.h"
#include "mpi.h"

int main(int argc,char** argv)
{
    int          taskid, ntasks;
    MPI_Status   status;
    MPI_Request  send_request,recv_request;
    int          ierr,i,j,itask,recvtaskid;
    int          buffsize;
    double       *sendbuff,*recvbuff;
    double       sendbuffsum,recvbuffsum;
    double       sendbuffsums[1024],recvbuffsums[1024];
    double       inittime,totaltime,recvtime,recvtimes[1024];
   
    /* MPI初始化 */
    MPI_Init(&argc, &argv);

    /* 获得MPI进程数量以及进程编号 */
    MPI_Comm_rank(MPI_COMM_WORLD,&taskid);
    MPI_Comm_size(MPI_COMM_WORLD,&ntasks);

    /* 从程序运行参数中获得buffer的大小 */
    buffsize=atoi(argv[1]);

    /* 打印该例的相关信息 */
    if ( taskid == 0 ){
        printf("\n\n");
        printf("##########################################################\n");
        printf(" Example 1 \n\n");
        printf(" Point-to-point Communication: MPI_Isend MPI_Irecv \n");
        printf(" Vector size: %d\n",buffsize);
        printf(" Number of tasks: %d\n\n",ntasks);
        printf("##########################################################\n");
        printf("                --> BEFORE COMMUNICATION <--\n\n");
    }

    /* 申请内存 */ 
    sendbuff=(double *)malloc(sizeof(double)*buffsize);
    recvbuff=(double *)malloc(sizeof(double)*buffsize);

    /*=============================================================*/
    /* 向量初始化 */
    srand((unsigned)time( NULL ) + taskid);
    for(i=0;i<buffsize;i++){
        sendbuff[i]=(double)rand()/RAND_MAX;
    }

    /* 打印初始化信息 */
    sendbuffsum=0.0;
    for(i=0;i<buffsize;i++){
        sendbuffsum += sendbuff[i];
    }   
    ierr=MPI_Gather(&sendbuffsum,1,MPI_DOUBLE,sendbuffsums,1, MPI_DOUBLE,0,MPI_COMM_WORLD);                  
    if(taskid == 0){
        for(itask=0; itask<ntasks; itask++){
            recvtaskid = itask+1;
            if(itask == (ntasks-1))
                recvtaskid= 0;
            printf("Task %d : Sum of vector sent to %d = %lf\n",itask,recvtaskid,sendbuffsums[itask]);
        }  
    }

    /* 进行通信 */

    inittime = MPI_Wtime();

    if ( taskid == 0 ){
        printf("\n Init time : %f seconds\n\n",inittime); 
        ierr = MPI_Isend(sendbuff,buffsize,MPI_DOUBLE,taskid+1,0,MPI_COMM_WORLD,&send_request);  
        ierr = MPI_Irecv(recvbuff,buffsize,MPI_DOUBLE,ntasks-1,MPI_ANY_TAG,MPI_COMM_WORLD,&recv_request);
        recvtime = MPI_Wtime();
    }
    else if( taskid == ntasks-1 ){
        ierr = MPI_Isend(sendbuff,buffsize,MPI_DOUBLE,0,0,MPI_COMM_WORLD,&send_request);   
        ierr = MPI_Irecv(recvbuff,buffsize,MPI_DOUBLE,taskid-1,MPI_ANY_TAG,MPI_COMM_WORLD,&recv_request);      
        recvtime = MPI_Wtime();
    }
    else{
        ierr = MPI_Isend(sendbuff,buffsize,MPI_DOUBLE,taskid+1,0,MPI_COMM_WORLD,&send_request);
        ierr = MPI_Irecv(recvbuff,buffsize,MPI_DOUBLE,taskid-1,MPI_ANY_TAG,MPI_COMM_WORLD,&recv_request);
        recvtime = MPI_Wtime();
    }

    ierr=MPI_Wait(&send_request,&status);
    ierr=MPI_Wait(&recv_request,&status);

    totaltime = MPI_Wtime() - inittime;

    /* 输出通信结束后结果 */

    recvbuffsum=0.0;
    for(i=0;i<buffsize;i++){
        recvbuffsum += recvbuff[i];
    }   

    ierr=MPI_Gather(&recvbuffsum,1,MPI_DOUBLE,recvbuffsums,1, MPI_DOUBLE,0,MPI_COMM_WORLD);
                   
    ierr=MPI_Gather(&recvtime,1,MPI_DOUBLE,recvtimes,1, MPI_DOUBLE,0,MPI_COMM_WORLD);

    if(taskid==0){
        printf("##########################################################\n\n");
        printf("                --> AFTER COMMUNICATION <-- \n\n");
        for(itask=0;itask<ntasks;itask++){
            printf("Task %d : Sum of received vector= %lf : Time=%f seconds\n",
            itask,recvbuffsums[itask],recvtimes[itask]);
        }  
        printf("\n");
        printf("##########################################################\n\n");
        printf(" Communication time : %f seconds\n\n",totaltime);  
        printf("##########################################################\n\n");
    }

    /* 释放资源 */
    free(recvbuff);
    free(sendbuff);

    /* MPI程序结束 */
    MPI_Finalize();

}