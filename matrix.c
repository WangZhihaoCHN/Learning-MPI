
#include <stdio.h>
#include "mpi.h"
#define N 200                     //矩阵的阶数
#define MASTER 0                  //主进程号
#define FROM_MASTER 1             //设置消息类型
#define FROM_WORKER 2             //设置消息类型
MPI_Status status;
void main(int argc,char **argv)
{
    int numtasks,                   //进程总数
    taskid,                    //进程标识
    numworkers,                //从进程数目
    source,                    //消息源
    dest,                      //消息目的地
    nbytes,
    mtype,                     //消息类型
    rows,
    averow,extra,offset,       //从进程所分的行数
    i,j,k,t,
    count;
    double A[N][N],B[N][N];
    long long C[N][N];
    long long sumand = 0;           //求的乘积矩阵的所有元素的和
    double starttime,endtime;
    MPI_Init(&argc,&argv);   // 初始化MPI环境
    MPI_Comm_rank(MPI_COMM_WORLD,&taskid);//标识各个MPI进程 ，告诉调用该函数进程的当前进程号
    MPI_Comm_size(MPI_COMM_WORLD,&numtasks);//用来标识相应进程组中有多少个进程
    numworkers = numtasks-1;     //从进程数目
    
    /* 程序采用主从模式，以下为主进程程序 */
    
    if(taskid==MASTER)
    {
        printf("--------主进程开始对矩阵A、B初始化\n");
        //对A矩阵B矩阵 进行初始化赋值
        for(i=0;i<N;i++)
        {
            t = i + 1;
            for(j=0;j<N;j++)
            {
                A[i][j]= t ++ ;
                B[i][j]= 1;
            }
        }
        printf("--------主进程开始对从进程发送数据\n");
        /*将数据(A矩阵B矩阵)发送到从进程*/
        averow=N/numworkers;   // 每个从进程所得到的行数
        extra=N%numworkers;
        offset=0;
        mtype=FROM_MASTER;
        for(dest=1;dest<=numworkers;dest++)
        {
            rows=(dest<=extra)?  averow+1:averow;
            //发送标识每个从进程开始寻找数据矩阵的偏移地址
            MPI_Send(&offset,1,MPI_INT,dest,mtype,MPI_COMM_WORLD);
            //发送每个从进程需要计算的行数
            MPI_Send(&rows,1,MPI_INT,dest,mtype,MPI_COMM_WORLD);
            //发送每个从进程 count=rows*N 比特流偏移量
            count=rows*N;
            //发送 矩阵 A
            MPI_Send(&A[offset][0],count,MPI_DOUBLE,dest,mtype,MPI_COMM_WORLD);
            count=N*N;
            //发送矩阵 B
            MPI_Send(&B,count,MPI_DOUBLE,dest,mtype,MPI_COMM_WORLD);
            offset=offset+rows;
        }
        printf("--------主进程等待从进程计算结果\n");
        /*等待接收从进程计算结果*/
        starttime = MPI_Wtime(); // 矩阵开始计算的时间
        mtype=FROM_WORKER;
        for(i=1;i<=numworkers;i++)
        {
            source=i; // 主进程 接受 从 从进程上发送的数据 ( C 矩阵)
            printf("主进程接受从进程号为 = %d\n",i);
            MPI_Recv(&offset,1,MPI_INT,source,mtype,MPI_COMM_WORLD,&status);
            MPI_Recv(&rows,1,MPI_INT,source,mtype,MPI_COMM_WORLD,&status);
            count=rows*N;
            MPI_Recv(&C[offset][0],count,MPI_DOUBLE,source,mtype,MPI_COMM_WORLD,&status);
        }
        endtime =  MPI_Wtime(); // 矩阵 结束计算的时间
        printf("********两矩阵相乘的时间为 :=%f\n",endtime - starttime);
        
        /*主进程计算 结果矩阵的 所有元素的和 为验证结果*/
        
        for(i=0;i<N;i++)
        {
            for(j=0;j<N;j++)
                sumand += C[i][j];
        }
        printf("--------主进程对矩阵C求和 sumand= %lld\n",sumand);
    }
    /* 从进程 接受数据 并 计算乘积 */
    if(taskid>MASTER)
    {
        mtype=FROM_MASTER;
        source=MASTER;
        //接受主进程发送的数据偏移值
        printf("\n++++++++该从进程开始从主进程上接受数据\n");
        MPI_Recv(&offset,1,MPI_INT,source,mtype,MPI_COMM_WORLD,&status);
        printf("++++++++该从进程的偏移量为：=%d\n",offset);
        //接收主进程发送到从进程需要计算的 行数
        MPI_Recv(&rows,1,MPI_INT,source,mtype,MPI_COMM_WORLD,&status);
        printf("++++++++该从进程需要计算的行数为: =%d\n",rows);
        
        count=rows*N;//接受矩阵A
        MPI_Recv(&A,count,MPI_DOUBLE,source,mtype,MPI_COMM_WORLD,&status);
        
        count=N*N;  //接受矩阵B
        MPI_Recv(&B,count,MPI_DOUBLE,source,mtype,MPI_COMM_WORLD,&status);
        
        for(k=0;k<N;k++)  //计算乘积
            for(i=0;i<rows;i++)
            {
                C[i][k]= 0.0;
                for(j=0;j<N;j++)
                    C[i][k] = C[i][k] + A[i][j] * B[j][k];
            }
        mtype=FROM_WORKER; // 把从矩阵计算乘积的 结果发送到 主进程上
        MPI_Send(&offset,1,MPI_INT,MASTER,mtype,MPI_COMM_WORLD);
        MPI_Send(&rows,1,MPI_INT,MASTER,mtype,MPI_COMM_WORLD);
        MPI_Send(&C,rows*N,MPI_DOUBLE,MASTER,mtype,MPI_COMM_WORLD);
    }
    MPI_Finalize();
}
