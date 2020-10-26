#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <clocale>
#include "mpi.h"

int main(int argc, char* argv[])
{
    //Функция Ring, проходит ring раз
    //Потом суммируются итоговые значения на потоках
    int ring = 2; //Число повторов
    int ProcNum, ProcRank, ProcFrom, ProcTo;
    MPI_Status Status;
    //char mess[] = "abcdef";
    //char recv[10];
    int mess = 0, recv = 0, sum = 0;

    //Инициализация среды
    MPI_Init(&argc, &argv);
    //Определение кол-ва процессов
    MPI_Comm_size(MPI_COMM_WORLD, &ProcNum);
    //Определение ранга процесса
    MPI_Comm_rank(MPI_COMM_WORLD, &ProcRank);

    if (ProcRank == 0)
    {
        //Действия, выполняемые только процессом с рангом 0
        MPI_Send(&mess, 1, MPI_INT, 1, 0, MPI_COMM_WORLD);
        printf("\n Start send:  %d -> %d", ProcRank, 1);
    }
    else //Сообщение, отправляемое всеми процессами, кроме процесса с рангом 0
    {
        for (int i = 0; i < ring; i++)
        {
            //Не первый круг и 1-й процесс
            if (i > 0 && ProcRank == 1)
                ProcFrom = ProcNum - 1; //От последнего 1-му
            else
                ProcFrom = ProcRank - 1; //От предыдущих
            MPI_Recv(&recv, 1, MPI_INT, ProcFrom, 0, MPI_COMM_WORLD, &Status);
            recv++;
            printf("\n Recv[%d]:  %d -> %d, %d+1", i, ProcFrom, ProcRank, recv - 1);

            //Если последний процесс
            if (ProcRank == ProcNum - 1)
                ProcTo = 1; //От последнего 1-му
            else
                ProcTo = ProcRank + 1; //Иначе следующему
            MPI_Send(&recv, 1, MPI_INT, ProcTo, 0, MPI_COMM_WORLD);
            printf("\n Send[%d]:  %d -> %d", i, ProcRank, ProcTo);
        }
    }

    // Суммируем последние результаты на процессах
    MPI_Reduce(&recv, &sum, 1, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);
    // Ждем все процессы, чтоб просуммировать все значения
    MPI_Barrier(MPI_COMM_WORLD);
    if (ProcRank == 0)
        printf("\n Result = %d", sum);
    printf("\n");

    //Завершение
    MPI_Finalize();
    return 0;

    //Функция Ring: 0 --> 1, 1-->2, 2-->3 и т.д. --> 1
    //И так ring раз
    /*int ring = 2; //Число повторов
    int ProcNum, ProcRank, ProcFrom, ProcTo;
    MPI_Status Status;
    char mess[] = "abcdef";
    char recv[10];
    //int mess = 5, recv = 0;

    //Инициализация среды
    MPI_Init(&argc, &argv);
    //Определение кол-ва процессов
    MPI_Comm_size(MPI_COMM_WORLD, &ProcNum);
    //Определение ранга процесса
    MPI_Comm_rank(MPI_COMM_WORLD, &ProcRank);

    if (ProcRank == 0)
    {
        //Действия, выполняемые только процессом с рангом 0
        MPI_Send(&mess, _countof(mess), MPI_CHAR, 1, 0, MPI_COMM_WORLD);
        printf("\n Start send:  %d -> %d", ProcRank, 1);
    }
    else //Сообщение, отправляемое всеми процессами, кроме процесса с рангом 0
    {
        for (int i = 0; i < ring; i++)
        {
            //Не первый круг и 1-й процесс
            if (i > 0 && ProcRank == 1) 
                ProcFrom = ProcNum - 1; //От последнего 1-му
            else 
                ProcFrom = ProcRank - 1; //От предыдущих
            MPI_Recv(&recv, _countof(recv), MPI_CHAR, ProcFrom, 0, MPI_COMM_WORLD, &Status);
            printf("\n Recv[%d]:  %d -> %d, %s", i, ProcFrom, ProcRank, recv);

            //Если последний процесс
            if (ProcRank == ProcNum - 1) 
                ProcTo = 1; //От последнего 1-му
            else 
                ProcTo = ProcRank + 1; //Иначе следующему
            MPI_Send(&recv, _countof(recv), MPI_CHAR, ProcTo, 0, MPI_COMM_WORLD);
            printf("\n Send[%d]:  %d -> %d", i, ProcRank, ProcTo);
        }
    }
    printf("\n");
    //Завершение
    MPI_Finalize(); 
    return 0;*/
}