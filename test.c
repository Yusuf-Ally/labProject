#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <omp.h>
#include "lsdtree.h"
#include "kdtree.h"

#define GRID_SIZE 100000
#define MAX_RECT_SIZE 0.10*GRID_SIZE
#define SEARCH_RECT_SIZE 0.50*GRID_SIZE
#define NUM_OF_DIMS 2
#define NUM_OF_RECORDS 10000
#define SIZE_OF_STEP 1000
#define NUM_OF_AVERAGE 100
#define SEARCH_RECT_PERC_STEP 10
#define NUM_OF_CLUSTERS 20


int __srand_done = 0;

double ** generate_rects(int numofrects, int numofdims, int gridsize, int rectsize)
{
    if (!__srand_done)
    {
        srand((unsigned int)time(NULL));
        __srand_done = 1;
    }

     double * arrvalues = malloc(sizeof(double) * (numofdims * 2) * numofrects);
     double ** values = malloc(sizeof(double*) * numofrects);

    for (int i = 0; i < numofrects; i++)
    {
        for (int j = 0; j < numofdims; j++)
        {
            int pos = i * (numofdims * 2) + j * 2;
            arrvalues[pos] = rand() % gridsize;
            arrvalues[pos+1] = arrvalues[pos] + rand() % rectsize;
        }

        values[i] = arrvalues + i * (numofdims*2);
    }

    return values;
}

int randnorm()
{
    int r = 0;

    for (int i = 0; i < 20; i++)
        r += rand();

    return r;
}

double ** generate_clustered_rects(int numofrects, int numofdims, int gridsize, int rectsize, int numofclusters)
{
    if (!__srand_done)
    {
        srand((unsigned int)time(NULL));
        __srand_done = 1;
    }

     double * arrvalues = malloc(sizeof(double) * (numofdims * 2) * numofrects);
     double ** values = malloc(sizeof(double*) * numofrects);

    int valuespercluster = numofrects / numofclusters;

    double dv = (double)RAND_MAX / gridsize;

    int ccentre[numofdims];

    int nval = 0;

    for (int d = 0; d < numofclusters; d++)
    {
        for (int i = 0; i < numofdims; i++)
            ccentre[i] = rand() % gridsize;

        for (int i = 0; i < valuespercluster; i++)
        {
            for (int j = 0; j < numofdims; j++)
            {
                int pos = nval * (numofdims * 2) + j * 2;
                arrvalues[pos] = ccentre[j] + (randnorm() / dv - gridsize / 2);

                if (arrvalues[pos] < 0)
                    arrvalues[pos] = 0;

                arrvalues[pos+1] = arrvalues[pos] + rand() % rectsize;
            }

            values[nval] = arrvalues + nval * (numofdims*2);
            nval += 1;
        }
    }

    for (int i = nval; i < numofrects; i++)
    {
        for (int j = 0; j < numofdims; j++)
        {
            int pos = i * (numofdims * 2) + j * 2;
            arrvalues[pos] = rand() % gridsize;
            arrvalues[pos+1] = arrvalues[pos] + rand() % rectsize;
        }

        values[i] = arrvalues + i * (numofdims*2);
    }

    return values;
}

void free_rects(double ** values)
{
    //free(values[0]);
    free(values);
}

void num_of_records_test()
{
    int grid_size = GRID_SIZE;
    int rect_size = MAX_RECT_SIZE;
    int search_rect_size = SEARCH_RECT_SIZE;
    int num_of_dims = NUM_OF_DIMS;
    int max_num_of_records = NUM_OF_RECORDS;
    int size_of_steps = SIZE_OF_STEP;
    int num_of_average = NUM_OF_AVERAGE;

    int num_of_cts_dims = num_of_dims * 2;

    printf("Number of rects test starting\n");

    for (int inum = size_of_steps; inum <= max_num_of_records; inum+=size_of_steps)
    {
        double ** values = generate_rects(inum,num_of_dims,grid_size,rect_size);

        double searchrect[num_of_cts_dims];

        searchrect[0] = 0;
        searchrect[1] = search_rect_size * grid_size / 100.0;

        for (int isr = 1; isr < num_of_dims; isr++)
        {
            searchrect[2*isr] = 0;
            searchrect[2*isr+1] = grid_size;
        }

        struct kdtree * kdtree = kd_create(values,num_of_cts_dims, inum);

        struct lsdtree * lsdtree = lsd_create(values,num_of_cts_dims, inum);

        double kdaverage = 0.0;
        for (int i = 0; i < num_of_average; i++)
        {
            double start = omp_get_wtime();
            int count = kd_par_cts_range_count2(kdtree, searchrect);
            kdaverage += omp_get_wtime() - start;
        }
        kdaverage /= num_of_average;

        double lsdaverage = 0.0;
        for (int i = 0; i < num_of_average; i++)
        {
            double start = omp_get_wtime();
            int count = lsd_par_cts_range_count2(lsdtree, searchrect);
            lsdaverage += omp_get_wtime() - start;
        }
        lsdaverage /= num_of_average;


        double skdaverage = 0.0;
        for (int i = 0; i < num_of_average; i++)
        {
            double start = omp_get_wtime();
            int count = kd_cts_range_count(kdtree, searchrect);
            skdaverage += omp_get_wtime() - start;
        }
        skdaverage /= num_of_average;

        double slsdaverage = 0.0;
        for (int i = 0; i < num_of_average; i++)
        {
            double start = omp_get_wtime();
            int count = lsd_cts_range_count(lsdtree, searchrect);
            slsdaverage += omp_get_wtime() - start;
        }
        slsdaverage /= num_of_average;

        double kdspeedup =  skdaverage / kdaverage;
        double lsdspeedup = slsdaverage / lsdaverage;

        printf("For %d values:\n", inum);

        printf("KD-Tree : %f s  Speedup = %f \n", kdaverage, kdspeedup);
        printf("LSD-Tree : %f s Speedup = %f \n\n", lsdaverage, lsdspeedup);

        free_rects(values);
    }
}

void search_rect_size_test()
{
    int grid_size = GRID_SIZE;
    int rect_size = MAX_RECT_SIZE;
    int search_rect_size = SEARCH_RECT_SIZE;
    int num_of_dims = NUM_OF_DIMS;
    int max_num_of_records = NUM_OF_RECORDS;
    int num_of_average = NUM_OF_AVERAGE;
    int search_rect_step = SEARCH_RECT_PERC_STEP;

    int num_of_cts_dims = num_of_dims * 2;

    printf("Search rect size test starting\n");

    double ** values = generate_rects(max_num_of_records,num_of_dims,grid_size,rect_size);

    struct kdtree * kdtree = kd_create(values,num_of_cts_dims, max_num_of_records);

    struct lsdtree * lsdtree = lsd_create(values,num_of_cts_dims, max_num_of_records);


    for (int iperc = search_rect_step; iperc <=  100; iperc+= search_rect_step)
    {
        double searchrect[num_of_cts_dims];

        searchrect[0] = 0;
        searchrect[1] = iperc * grid_size / 100.0;

        for (int isr = 1; isr < num_of_dims; isr++)
        {
            searchrect[2*isr] = 0;
            searchrect[2*isr+1] = grid_size;
        }

        double kdaverage = 0.0;
        for (int i = 0; i < num_of_average; i++)
        {
            double start = omp_get_wtime();
            int count = kd_par_cts_range_count2(kdtree, searchrect);
            kdaverage += omp_get_wtime() - start;
        }
        kdaverage /= num_of_average;

        double lsdaverage = 0.0;
        for (int i = 0; i < num_of_average; i++)
        {
            double start = omp_get_wtime();
            int count = lsd_par_cts_range_count2(lsdtree, searchrect);
            lsdaverage += omp_get_wtime() - start;
        }
        lsdaverage /= num_of_average;

        printf("For %d %% of grid size:\n", iperc);

        printf("KD-Tree : %f s \n", kdaverage);
        printf("LSD-Tree : %f s \n", lsdaverage);
    }



}

void clustered_data_test()
{
    int grid_size = GRID_SIZE;
    int rect_size = MAX_RECT_SIZE;
    int search_rect_size = SEARCH_RECT_SIZE;
    int num_of_dims = NUM_OF_DIMS;
    int max_num_of_records = NUM_OF_RECORDS;
    int size_of_steps = SIZE_OF_STEP;
    int num_of_average = NUM_OF_AVERAGE;
    int num_of_clusters = NUM_OF_CLUSTERS;

    int num_of_cts_dims = num_of_dims * 2;

    printf("Clustered data test starting\n");

    for (int i = 1; i <= num_of_clusters; i++)
    {
        double ** values = generate_clustered_rects(max_num_of_records,num_of_dims,grid_size,rect_size, i);

        double searchrect[num_of_cts_dims];

        searchrect[0] = 0;
        searchrect[1] = grid_size;
        searchrect[2] = 0;
        searchrect[3] = search_rect_size;

        struct kdtree * kdtree = kd_create(values,num_of_cts_dims, max_num_of_records);

        struct lsdtree * lsdtree = lsd_create(values,num_of_cts_dims, max_num_of_records);

        double kdaverage = 0.0;
        for (int i = 0; i < num_of_average; i++)
        {
            double start = omp_get_wtime();
            int count = kd_par_cts_range_count2(kdtree, searchrect);
            kdaverage += omp_get_wtime() - start;
        }
        kdaverage /= num_of_average;

        double lsdaverage = 0.0;
        for (int i = 0; i < num_of_average; i++)
        {
            double start = omp_get_wtime();
            int count = lsd_par_cts_range_count2(lsdtree, searchrect);
            lsdaverage += omp_get_wtime() - start;
        }
        lsdaverage /= num_of_average;

        printf("For %d clusters:\n", i);

        printf("KD-Tree : %f s \n", kdaverage);
        printf("LSD-Tree : %f s \n", lsdaverage);

        free_rects(values);
    }
}

void clustered_num_of_records_test()
{
    int grid_size = GRID_SIZE;
    int rect_size = MAX_RECT_SIZE;
    int search_rect_size = SEARCH_RECT_SIZE;
    int num_of_dims = NUM_OF_DIMS;
    int max_num_of_records = NUM_OF_RECORDS;
    int size_of_steps = SIZE_OF_STEP;
    int num_of_average = NUM_OF_AVERAGE;

    int num_of_cts_dims = num_of_dims * 2;

    printf("Clustered number of rects test starting\n");

    for (int inum = size_of_steps; inum <= max_num_of_records; inum+=size_of_steps)
    {
        double ** values = generate_clustered_rects(inum,num_of_dims,grid_size,rect_size, 4);

        double searchrect[num_of_cts_dims];

        searchrect[0] = 0;
        searchrect[1] = search_rect_size * grid_size / 100.0;

        for (int isr = 1; isr < num_of_dims; isr++)
        {
            searchrect[2*isr] = 0;
            searchrect[2*isr+1] = grid_size;
        }

        struct kdtree * kdtree = kd_create(values,num_of_cts_dims, inum);

        struct lsdtree * lsdtree = lsd_create(values,num_of_cts_dims, inum);

        double kdaverage = 0.0;
        for (int i = 0; i < num_of_average; i++)
        {
            double start = omp_get_wtime();
            int count = kd_par_cts_range_count2(kdtree, searchrect);
            kdaverage += omp_get_wtime() - start;
        }
        kdaverage /= num_of_average;

        double lsdaverage = 0.0;
        for (int i = 0; i < num_of_average; i++)
        {
            double start = omp_get_wtime();
            int count = lsd_par_cts_range_count2(lsdtree, searchrect);
            lsdaverage += omp_get_wtime() - start;
        }
        lsdaverage /= num_of_average;

        printf("For %d values:\n", inum);

        printf("KD-Tree : %f s \n", kdaverage);
        printf("LSD-Tree : %f s \n", lsdaverage);

        free_rects(values);
    }
}
