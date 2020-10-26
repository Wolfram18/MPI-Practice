#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <clocale>
#include "mpi.h"

int main(int argc, char* argv[])
{
    //������� Ring, �������� ring ���
    //����� ����������� �������� �������� �� �������
    int ring = 2; //����� ��������
    int ProcNum, ProcRank, ProcFrom, ProcTo;
    MPI_Status Status;
    //char mess[] = "abcdef";
    //char recv[10];
    int mess = 0, recv = 0, sum = 0;

    //������������� �����
    MPI_Init(&argc, &argv);
    //����������� ���-�� ���������
    MPI_Comm_size(MPI_COMM_WORLD, &ProcNum);
    //����������� ����� ��������
    MPI_Comm_rank(MPI_COMM_WORLD, &ProcRank);

    if (ProcRank == 0)
    {
        //��������, ����������� ������ ��������� � ������ 0
        MPI_Send(&mess, 1, MPI_INT, 1, 0, MPI_COMM_WORLD);
        printf("\n Start send:  %d -> %d", ProcRank, 1);
    }
    else //���������, ������������ ����� ����������, ����� �������� � ������ 0
    {
        for (int i = 0; i < ring; i++)
        {
            //�� ������ ���� � 1-� �������
            if (i > 0 && ProcRank == 1)
                ProcFrom = ProcNum - 1; //�� ���������� 1-��
            else
                ProcFrom = ProcRank - 1; //�� ����������
            MPI_Recv(&recv, 1, MPI_INT, ProcFrom, 0, MPI_COMM_WORLD, &Status);
            recv++;
            printf("\n Recv[%d]:  %d -> %d, %d+1", i, ProcFrom, ProcRank, recv - 1);

            //���� ��������� �������
            if (ProcRank == ProcNum - 1)
                ProcTo = 1; //�� ���������� 1-��
            else
                ProcTo = ProcRank + 1; //����� ����������
            MPI_Send(&recv, 1, MPI_INT, ProcTo, 0, MPI_COMM_WORLD);
            printf("\n Send[%d]:  %d -> %d", i, ProcRank, ProcTo);
        }
    }

    // ��������� ��������� ���������� �� ���������
    MPI_Reduce(&recv, &sum, 1, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);
    // ���� ��� ��������, ���� �������������� ��� ��������
    MPI_Barrier(MPI_COMM_WORLD);
    if (ProcRank == 0)
        printf("\n Result = %d", sum);
    printf("\n");

    //����������
    MPI_Finalize();
    return 0;

    //������� Ring: 0 --> 1, 1-->2, 2-->3 � �.�. --> 1
    //� ��� ring ���
    /*int ring = 2; //����� ��������
    int ProcNum, ProcRank, ProcFrom, ProcTo;
    MPI_Status Status;
    char mess[] = "abcdef";
    char recv[10];
    //int mess = 5, recv = 0;

    //������������� �����
    MPI_Init(&argc, &argv);
    //����������� ���-�� ���������
    MPI_Comm_size(MPI_COMM_WORLD, &ProcNum);
    //����������� ����� ��������
    MPI_Comm_rank(MPI_COMM_WORLD, &ProcRank);

    if (ProcRank == 0)
    {
        //��������, ����������� ������ ��������� � ������ 0
        MPI_Send(&mess, _countof(mess), MPI_CHAR, 1, 0, MPI_COMM_WORLD);
        printf("\n Start send:  %d -> %d", ProcRank, 1);
    }
    else //���������, ������������ ����� ����������, ����� �������� � ������ 0
    {
        for (int i = 0; i < ring; i++)
        {
            //�� ������ ���� � 1-� �������
            if (i > 0 && ProcRank == 1) 
                ProcFrom = ProcNum - 1; //�� ���������� 1-��
            else 
                ProcFrom = ProcRank - 1; //�� ����������
            MPI_Recv(&recv, _countof(recv), MPI_CHAR, ProcFrom, 0, MPI_COMM_WORLD, &Status);
            printf("\n Recv[%d]:  %d -> %d, %s", i, ProcFrom, ProcRank, recv);

            //���� ��������� �������
            if (ProcRank == ProcNum - 1) 
                ProcTo = 1; //�� ���������� 1-��
            else 
                ProcTo = ProcRank + 1; //����� ����������
            MPI_Send(&recv, _countof(recv), MPI_CHAR, ProcTo, 0, MPI_COMM_WORLD);
            printf("\n Send[%d]:  %d -> %d", i, ProcRank, ProcTo);
        }
    }
    printf("\n");
    //����������
    MPI_Finalize(); 
    return 0;*/
}