#include <iomanip>
#include "itkImageFileReader.h"
#include "itkImageFileWriter.h"
#include "itkCommand.h"
#include "itkSimpleFilterWatcher.h"
#include "itkChangeInformationImageFilter.h"

//#include "plotutils.h"
#include "ioutils.h"

#include "itkBinaryThresholdImageFilter.h"
#include "itkMorphologicalSignedDistanceTransformImageFilter.h"
//#include "itkMorphologicalDistanceTransformImageFilter.h"
#include "itkSignedMaurerDistanceMapImageFilter.h"

#include "itkDanielssonDistanceMapImageFilter.h"

#include "itkTimeProbe.h"
#include "itkMultiThreader.h"

int
main(int argc, char * argv[])
{
  int iterations = 1;

  if (argc != 8)
  {
    std::cerr << "Usage: " << argv[0] << " inputimage threshold outsideval outim1 outim2 outim3" << std::endl;
    return (EXIT_FAILURE);
  }

  iterations = std::stoi(argv[1]);

  // itk::MultiThreader::SetGlobalMaximumNumberOfThreads(1);
  constexpr int dim = 3;

  using PType = unsigned char;
  using IType = itk::Image<PType, dim>;
  using FType = itk::Image<float, dim>;

  IType::Pointer inputOrig = readIm<IType>(argv[1]);

  using ChangeType = itk::ChangeInformationImageFilter<IType>;
  ChangeType::Pointer changer = ChangeType::New();
  changer->SetInput(inputOrig);
  ChangeType::SpacingType newspacing;

  newspacing[0] = 0.5;
  newspacing[1] = 0.25;
  newspacing[2] = 0.75;
  newspacing.Fill(1);

  changer->SetOutputSpacing(newspacing);
  changer->ChangeSpacingOn();

  IType::Pointer input = changer->GetOutput();

  // threshold the input to create a mask
  using ThreshType = itk::BinaryThresholdImageFilter<IType, IType>;
  ThreshType::Pointer thresh = ThreshType::New();
  thresh->SetInput(input);

  thresh->SetUpperThreshold(std::stoi(argv[2]));
  thresh->SetInsideValue(0);
  thresh->SetOutsideValue(255);
  writeIm<IType>(thresh->GetOutput(), argv[4]);
  // now to apply the signed distance transform
  std::cout << "Finished loading etc" << std::endl;

  using FilterType = itk::MorphologicalSignedDistanceTransformImageFilter<IType, FType>;

  FilterType::Pointer filter = FilterType::New();

  filter->SetInput(thresh->GetOutput());
  filter->SetOutsideValue(std::stoi(argv[3]));
  filter->SetUseImageSpacing(true);
  filter->SetParabolicAlgorithm(FilterType::CONTACTPOINT);
  // filter->UseContactPointOn();
  itk::TimeProbe ParabolicCP, ParabolicInt, MaurerT, DanielssonT;

  std::cout << "ParabolicCP ParabolicIntersection  Maurer   Danielsson" << std::endl;

  constexpr unsigned pTESTS = 10;
  for (unsigned repeats = 0; repeats < pTESTS; repeats++)
  {
    ParabolicCP.Start();
    filter->Modified();
    filter->Update();
    ParabolicCP.Stop();
  }

  filter->SetParabolicAlgorithm(FilterType::INTERSECTION);
  for (unsigned repeats = 0; repeats < pTESTS; repeats++)
  {
    ParabolicInt.Start();
    filter->Modified();
    filter->Update();
    ParabolicInt.Stop();
  }

  writeIm<FType>(filter->GetOutput(), argv[5]);

  using MaurerType = itk::SignedMaurerDistanceMapImageFilter<IType, FType>;
  MaurerType::Pointer maurer = MaurerType::New();
  maurer->SetInput(thresh->GetOutput());
  maurer->SetUseImageSpacing(true);
  maurer->SetSquaredDistance(false);
  for (unsigned repeats = 0; repeats < pTESTS; repeats++)
  {
    MaurerT.Start();
    maurer->Modified();
    maurer->Update();
    MaurerT.Stop();
  }
  writeIm<FType>(maurer->GetOutput(), argv[6]);

  constexpr unsigned TESTS = 10;

  using DanielssonType = itk::DanielssonDistanceMapImageFilter<IType, FType>;
  DanielssonType::Pointer daniel = DanielssonType::New();
  daniel->SetInput(thresh->GetOutput());
  daniel->SetUseImageSpacing(true);
  for (unsigned repeats = 0; repeats < 2; repeats++)
  {
    DanielssonT.Start();
    daniel->Modified();
    daniel->Update();
    DanielssonT.Stop();
  }
  writeIm<FType>(daniel->GetDistanceMap(), argv[7]);

  std::cout << std::setprecision(3) << ParabolicCP.GetMean() << "\t" << ParabolicInt.GetMean() << "\t"
            << MaurerT.GetMean() << "\t" << DanielssonT.GetMean() << std::endl;

  return EXIT_SUCCESS;
}
