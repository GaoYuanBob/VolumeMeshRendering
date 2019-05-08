#include "vtkSmartPointer.h"
#include "vtkMatrix4x4.h"
#include "vtkLookupTable.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkActor.h"
#include "vtkPointData.h"
#include "vtkPolyDataMapper.h"
#include "vtkOBJReader.h"
#include "vtkTransform.h"
#include "vtkProperty.h"
#include "vtkLODActor.h"
#include "vtkSmartVolumeMapper.h"
#include "vtkDICOMImageReader.h"
#include "vtkColorTransferFunction.h"
#include "vtkVolumeProperty.h"
#include "vtkPiecewiseFunction.h"
#include <vtkImageData.h>
#include <vtkAxesActor.h>
#include "vtkInteractorStyleTrackballCamera.h"

#include <iostream>
#include <fstream>
#include <string> 

// 前两个一定要加，不然会 no override found for ...
#include <vtkAutoInit.h>
VTK_MODULE_INIT(vtkRenderingVolumeOpenGL2);
VTK_MODULE_INIT(vtkRenderingOpenGL2);
VTK_MODULE_INIT(vtkInteractionStyle);

void render(const std::string& meshfile, const std::string& matrixfile, const std::string& volumefile)
{
	vtkAlgorithm * volumereader = nullptr;
	vtkImageData * input = nullptr;
	vtkDICOMImageReader * dicomReader = vtkDICOMImageReader::New();
	dicomReader->SetDirectoryName(volumefile.c_str());
	dicomReader->Update();
	input = dicomReader->GetOutput();
	volumereader = dicomReader;

	// Verify that we actually have a volume
	int dim[3];
	input->GetDimensions(dim);

	if (dim[0] < 2 || dim[1] < 2 || dim[2] < 2)
		cout << "Error loading data!" << endl;

	vtkSmartPointer<vtkOBJReader> reader = vtkSmartPointer<vtkOBJReader>::New();
	reader->SetFileName(meshfile.c_str());

	vtkSmartPointer<vtkPolyDataMapper> mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
	mapper->AddInputConnection(reader->GetOutputPort());

	vtkSmartPointer<vtkLODActor> actor = vtkSmartPointer<vtkLODActor>::New();

	vtkSmartPointer<vtkRenderWindowInteractor> renderWindowInteractor = vtkSmartPointer<vtkRenderWindowInteractor>::New();
	ifstream f;
	f.open(matrixfile, ios::in);
	if (!f.is_open()) {
		cout << "Erro when Loading Matrix File" << endl;
		return;
	}
	double ma[4][4];
	for (int i = 0; i < 4; i++)
		for (int j = 0; j < 4; j++)
			f >> ma[i][j];

	vtkSmartPointer<vtkMatrix4x4> m = vtkSmartPointer<vtkMatrix4x4>::New();
	for (int i = 0; i<4; i++)
		for (int j = 0; j<4; j++)
			m->SetElement(i, j, ma[i][j]);

	actor->SetMapper(mapper);

	m->Print(cout);

	vtkSmartPointer<vtkInteractorStyleTrackballCamera> style = vtkSmartPointer<vtkInteractorStyleTrackballCamera>::New();
	renderWindowInteractor->SetInteractorStyle(style);
	vtkSmartPointer<vtkRenderer> renderer = vtkSmartPointer<vtkRenderer>::New();
	vtkSmartPointer<vtkRenderWindow> renderWindow = vtkSmartPointer<vtkRenderWindow>::New();
	renderWindow->AddRenderer(renderer);
	renderWindow->SetInteractor(renderWindowInteractor);
	renderWindow->SetSize(1000, 800);
	renderWindowInteractor->Initialize();

	// Create our volume and mapper
	vtkVolume *volume = vtkVolume::New();
	vtkSmartVolumeMapper *volumemapper = vtkSmartVolumeMapper::New();
	//volume->bo
	volumemapper->SetInputConnection(volumereader->GetOutputPort());

	vtkColorTransferFunction *colorFun = vtkColorTransferFunction::New();
	vtkPiecewiseFunction *opacityFun = vtkPiecewiseFunction::New();

	// Create the property and attach the transfer functions
	vtkVolumeProperty *property = vtkVolumeProperty::New();
	property->SetColor(colorFun);
	property->SetScalarOpacity(opacityFun);
	property->SetInterpolationTypeToLinear();

	// connect up the volume to the property and the mapper
	volume->SetProperty(property);
	volume->SetMapper(volumemapper);
	colorFun->AddRGBSegment(0, 0.0, 0.0, 0.0, 255, 0.0, 0.0, 0.0);
	/*colorFun->AddRGBSegment(0.0, 1.0, 1.0, 1.0, 255.0, 1.0, 1.0, 1.0 );*/
	opacityFun->AddSegment(2048 - 0.9 * 4096, 0.0, 2048 + 0.1 * 4096, 0.8);
	volumemapper->SetBlendModeToMaximumIntensity();

	//getbound
	double a[6];

	volume->SetPosition(0, 0, 0);
	volume->GetBounds(a);
	for (auto i : a)
		cout << i << endl;
	renderer->AddViewProp(volume);

	renderer->AddViewProp(actor);
	actor->SetNumberOfCloudPoints(100);

	vtkSmartPointer<vtkTransform> tf = vtkSmartPointer<vtkTransform>::New();

	tf->Identity();
	a[0] = 0; a[1] = 0; a[2] = 0;
	actor->SetPosition(a);

	renderer->ResetCamera();
	actor->SetUserMatrix(m);
	actor->GetBounds(a);
	actor->GetProperty()->BackfaceCullingOn();

	renderer->SetBackground(1, 1, 1);

	renderWindow->Render();
	renderWindow->SetWindowName("可视化结果");
	renderWindowInteractor->Start();
}

int main()
{
	const std::string mesh_file = R"(..\..\Test_data\all_teeth.obj)";
	std::string matrix_file = R"(..\..\Test_data\reg.txt)";
	const std::string volume_file = R"(..\..\Test_data\CBCT_dicoms)";
	auto choice = 0;
	cin >> choice;
	if (choice == 0)	// use identity matrix
		matrix_file = R"(..\..\Test_data\identity_matrix.txt)";

	render(mesh_file, matrix_file, volume_file);

	return 0;
}