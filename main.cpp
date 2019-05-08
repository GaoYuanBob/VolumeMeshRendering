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
#include "vtkProperty.h"
#include "vtkLODActor.h"
#include "vtkSmartVolumeMapper.h"
#include "vtkDICOMImageReader.h"
#include "vtkColorTransferFunction.h"
#include "vtkVolumeProperty.h"
#include "vtkPiecewiseFunction.h"
#include "vtkImageData.h"
#include "vtkAxesActor.h"
#include "vtkInteractorStyleTrackballCamera.h"

#include <iostream>
#include <fstream>
#include <string> 

// 前两个一定要加，不然会 no override found for ...
#include <vtkAutoInit.h>
VTK_MODULE_INIT(vtkRenderingVolumeOpenGL2);
VTK_MODULE_INIT(vtkRenderingOpenGL2);
VTK_MODULE_INIT(vtkInteractionStyle);

void render(const std::string& mesh_file, const std::string& matrix_file, const std::string& volume_file)
{
/// 1、read CBCT dicom volume data
	auto dicomReader = vtkDICOMImageReader::New();
	dicomReader->SetDirectoryName(volume_file.c_str());
	dicomReader->Update();

	// Verify that we actually have a volume
	int dim[3];
	dicomReader->GetOutput()->GetDimensions(dim);

	if (dim[0] < 2 || dim[1] < 2 || dim[2] < 2)	cout << "Error loading data!" << endl;

/// 2、read mesh obj data
	auto mesh_reader = vtkSmartPointer<vtkOBJReader>::New();
	mesh_reader->SetFileName(mesh_file.c_str());

	auto mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
	mapper->AddInputConnection(mesh_reader->GetOutputPort());

/// 3、read transform matrix for mesh
	ifstream f;
	f.open(matrix_file, ios::in);
	if (!f.is_open()) {
		cout << "Error loading transform matrix file" << endl;
		return;
	}
	double tmp;
	auto mat = vtkSmartPointer<vtkMatrix4x4>::New();
	for (int i = 0; i<4; i++)
		for (int j = 0; j < 4; j++)
		{
			f >> tmp;
			mat->SetElement(i, j, tmp);
		}
	f.close();
	mat->Print(cout);

/// 4、Create property and volume_mapper
	auto volume_mapper = vtkSmartVolumeMapper::New();
	volume_mapper->SetInputConnection(dicomReader->GetOutputPort());

	auto color_func = vtkColorTransferFunction::New();
	auto opacity_func = vtkPiecewiseFunction::New();

	// Create the property and attach the transfer functions
	auto property = vtkVolumeProperty::New();
	property->SetColor(color_func);
	color_func->AddRGBSegment(0, 0.0, 0.0, 0.0, 255, 0.0, 0.0, 0.0);

	property->SetScalarOpacity(opacity_func);
	opacity_func->AddSegment(2048 - 0.9 * 4096, 0.0, 2048 + 0.1 * 4096, 0.8);

	property->SetInterpolationTypeToLinear();

	// connect up the volume to the property and the mapper
	auto volume = vtkVolume::New();
	volume->SetProperty(property);
	volume->SetMapper(volume_mapper);
	volume->SetPosition(0, 0, 0);
	
	volume_mapper->SetBlendModeToMaximumIntensity();

	// get bound of volume
	double bounds[6];
	volume->GetBounds(bounds);
	printf("Bounds of Volume is: (%.2lf, %.2lf, %.2lf) to (%.2lf, %.2lf, %.2lf)\n", 
		bounds[0], bounds[2], bounds[4], bounds[1], bounds[3], bounds[5]);

/// 5、get ready for rendering
	auto renderWindowInteractor = vtkSmartPointer<vtkRenderWindowInteractor>::New();
	// 不设置style控制会很奇怪
	renderWindowInteractor->SetInteractorStyle(vtkSmartPointer<vtkInteractorStyleTrackballCamera>::New());

	auto renderWindow = vtkSmartPointer<vtkRenderWindow>::New();
	auto renderer = vtkSmartPointer<vtkRenderer>::New();
	renderWindow->AddRenderer(renderer);
	renderWindow->SetInteractor(renderWindowInteractor);
	renderWindow->SetSize(1000, 800);
	renderWindowInteractor->Initialize();	// 先 SetInteractor()，后 Initialize()
	renderer->AddViewProp(volume);

	auto actor = vtkSmartPointer<vtkLODActor>::New();
	actor->SetMapper(mapper);
	renderer->AddViewProp(actor);
	renderer->ResetCamera();
	renderer->SetBackground(1, 1, 1);

	actor->SetNumberOfCloudPoints(100);
	actor->SetPosition(0.0, 0.0, 0.0);
	actor->SetUserMatrix(mat);
	actor->GetBounds(bounds);
	actor->GetProperty()->BackfaceCullingOn();

/// 6、render
	renderWindow->Render();
	renderWindow->SetWindowName("CBCT + Mesh Rendering");
	renderWindowInteractor->Start();
}

int main()
{
	const std::string mesh_file = R"(..\..\Test_data\all_teeth.obj)";	// mesh 模型路径
	const std::string matrix_file = R"(..\..\Test_data\reg.txt)";		// mesh 模型的变换矩阵，可以不用
	const std::string volume_file = R"(..\..\Test_data\CBCT_dicoms)";	// CBCT dicom 数据路径

	render(mesh_file, matrix_file, volume_file);

	return 0;
}



/*
 *	auto choice = 0;
	cin >> choice;
	if (choice == 0)	// use identity matrix
		matrix_file = R"(..\..\Test_data\identity_matrix.txt)";
 */