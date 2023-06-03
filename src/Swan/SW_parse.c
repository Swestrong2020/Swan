#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <stdint.h>

#include "SW_parse.h"
#include "SW_matrix.h"

#define DEBUG

// https://codereview.stackexchange.com/questions/151049/endianness-conversion-in-c
static inline uint32_t reverse32(uint32_t value) 
{
    return (((value & 0x000000FF) << 24) | ((value & 0x0000FF00) <<  8) | ((value & 0x00FF0000) >>  8) | ((value & 0xFF000000) >> 24));
}

/* reads a single uint32_t from stream and flips the endianness */
static inline uint32_t freadReverse32(FILE *stream)
{
    uint32_t val;
    fread(&val, sizeof(uint32_t), 1, stream);
    val = reverse32(val);
    return val;
}


const char lightValues[10] = { ' ', '.', ':', '-', '=', '+', '*', '#', '%', '@' };

void SW_printMNISTImage(uint8_t *pointer, int32_t rows, int32_t columns)
{
    uint32_t buflen = rows*columns+rows+8;
    char output[buflen];

    uint32_t index = 0, outIndex = 0;

    for (uint32_t i = 0; i < rows; i++)
    {
        for (uint32_t j = 0; j < columns; j++)
        {
            uint8_t lightval = (uint8_t)round((float)((float)pointer[index++]/(float)UINT8_MAX) * 10.0f);

            if (lightval > 9) lightval = 9;

            output[outIndex++] = lightValues[lightval];

            if (outIndex >= buflen) { fputs("buffer overflow in image printing\n", stderr); abort(); }
        }
        output[outIndex++] = '\n';

        if (outIndex >= buflen) { fputs("buffer overflow in image printing\n", stderr); abort(); }
    }

    output[outIndex] = '\0';
    fputs(output, stdout);
}

void SW_parseMNIST(
    uint32_t *_nTrainingLabels, uint32_t *_nTrainingImages, uint32_t *_nTestLabels, uint32_t *_nTestImages,
    uint8_t **_trainingLabels, uint8_t **_trainingImages, uint8_t **_testLabels, uint8_t **_testImages
)
{

    // funky way to handle the storing of data generated in this function into the arguments
    // to this function, storing is done after execution (see bottom of this arguable too long function -_-)
    int32_t nTrainingLabels = 0, nTrainingImages = 0, nTestLabels = 0, nTestImages = 0;
    uint8_t *trainingLabels = NULL, *trainingImages = NULL, *testLabels = NULL, *testImages = NULL;


    // parsing the MNIST dataset
    // ------------------------------------------------


    // training labels
    FILE *fTrainingLabels = fopen("MNISTdataset/train-labels-idx1-ubyte", "rb");

    // safety checks
    if (fTrainingLabels == NULL)
    {
        fputs("error opening training label file\n", stderr);
        abort();
    }

    int32_t trainingLabelsMagicBytes;

    trainingLabelsMagicBytes = (int32_t)freadReverse32(fTrainingLabels);
    nTrainingLabels = (int32_t)freadReverse32(fTrainingLabels);

    if (trainingLabelsMagicBytes != 2049 && trainingLabelsMagicBytes != 17301504)
    {
        fputs("training labels corrupted\n", stderr);
        abort();
    }

    trainingLabels = realloc(trainingLabels, sizeof(uint8_t) * nTrainingLabels);
    size_t trainingLabelsNRead = fread(trainingLabels, sizeof(uint8_t), nTrainingLabels, fTrainingLabels);
    
#ifdef DEBUG
    printf("read %zu bytes into trainingLabels\n", trainingLabelsNRead);
#endif

    fclose(fTrainingLabels);



    // training images
    FILE *fTrainingImages = fopen("MNISTdataset/train-images-idx3-ubyte", "rb");

    if (fTrainingImages == NULL)
    {
        fputs("error opening training images file\n", stderr);
        abort();
    }

    int32_t trainingImagesMagicBytes, trainingImagesRows, trainingImagesColumns;

    trainingImagesMagicBytes = (int32_t)freadReverse32(fTrainingImages);
    nTrainingImages = (int32_t)freadReverse32(fTrainingImages);
    trainingImagesRows = (int32_t)freadReverse32(fTrainingImages);
    trainingImagesColumns = (int32_t)freadReverse32(fTrainingImages);

    if (trainingImagesMagicBytes != 2051 && trainingImagesMagicBytes != 50855936)
    {
        fputs("training images corrupted\n", stderr);
        abort();
    }

    trainingImages = realloc(trainingImages, sizeof(uint8_t) * nTrainingImages * trainingImagesRows * trainingImagesColumns);
    size_t trainingImagesNRead = fread(trainingImages, sizeof(uint8_t), nTrainingImages * trainingImagesRows * trainingImagesColumns, fTrainingImages);

#ifdef DEBUG
    printf("read %zu bytes into trainingImages\n", trainingImagesNRead);
#endif

    fclose(fTrainingImages);



    // test labels
    FILE *fTestLabels = fopen("MNISTdataset/t10k-labels-idx1-ubyte", "rb");

    if (fTestLabels == NULL)
    {
        fputs("error opening test label file\n", stderr);
        abort();
    }

    int32_t testLabelsMagicBytes;

    testLabelsMagicBytes = (int32_t)freadReverse32(fTestLabels);
    nTestLabels = (int32_t)freadReverse32(fTestLabels);

    if (testLabelsMagicBytes != 2049 && testLabelsMagicBytes != 17301504)
    {
        fputs("test labels corrupted\n", stderr);
        abort();
    }

    testLabels = realloc(testLabels, sizeof(uint8_t) * nTestLabels);
    size_t testLabelsNRead = fread(testLabels, sizeof(uint8_t), nTestLabels, fTestLabels);

#ifdef DEBUG
    printf("read %zu bytes into testLabels\n", testLabelsNRead);
#endif

    fclose(fTestLabels);



    // test images
    FILE *fTestImages = fopen("MNISTdataset/t10k-images-idx3-ubyte", "rb");

    if (fTestImages == NULL)
    {
        fputs("error opening test images file\n", stderr);
        abort();
    }

    int32_t testImageMagicBytes, testImagesRows, testImagesColumns;

    testImageMagicBytes = (int32_t)freadReverse32(fTestImages);
    nTestImages = (int32_t)freadReverse32(fTestImages);
    testImagesRows = (int32_t)freadReverse32(fTestImages);
    testImagesColumns = (int32_t)freadReverse32(fTestImages);

    if (testImageMagicBytes != 2051 && testImageMagicBytes != 50855936)
    {
        fputs("test images corrupted\n", stderr);
        abort();
    }

    testImages = realloc(testImages, sizeof(uint8_t) * nTestImages * testImagesRows * testImagesColumns);
    size_t testImagesNRead = fread(testImages, sizeof(uint8_t), nTestImages * testImagesRows * testImagesColumns, fTestImages);

#ifdef DEBUG
    printf("read %zu bytes into testImages\n", testImagesNRead);
#endif


    fclose(fTestImages);


    
    // store values used in this function back into the passed arguments
    // dereference operator go brrrrrr
    (*_nTrainingLabels) = nTrainingLabels;
    (*_nTrainingImages) = nTrainingImages;
    (*_nTestLabels) = nTestLabels;
    (*_nTestImages) = nTestImages;
    
    (*_testLabels) = testLabels;
    (*_testImages) = testImages;
    (*_trainingLabels) = trainingLabels;
    (*_trainingImages) = trainingImages;
}

SWM_Matrix SW_MNISTImageToMatrix(uint8_t *imagePointer, int32_t rows, int32_t columns)
{
    SWM_Matrix out;
    SWM_initMatrix(&out, rows, columns);
}