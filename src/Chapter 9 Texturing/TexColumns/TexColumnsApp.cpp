
#include "../../Common/Camera.h"
#include "../../Common/d3dApp.h"
#include "../../Common/MathHelper.h"
#include "../../Common/UploadBuffer.h"
#include "../../Common/GeometryGenerator.h"

#include "imgui/imgui.h"
#include "imgui/imgui_impl_win32.h"
#include "imgui/imgui_impl_dx12.h"

#include "GBuffer.h"
#include "RenderingSystem.h"

#include <filesystem>
#include "FrameResource.h"
#include <iostream>

Camera cam;
using Microsoft::WRL::ComPtr;
using namespace DirectX;
using namespace DirectX::PackedVector;

#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "D3D12.lib")

// GLOBAL PARAMS

const int gNumFrameResources = 3;
float sputnik_speed = .15f;

float radius = 10.f;
int lights_needed = 5;
XMFLOAT2 centr = { 0.f, 0.0f };
float height = 10.f;
float spotlight_circle_strength = 2.f;

float orbitAngle = radius;

float camera_defaultposition[3] = { 0.f, 10.f, 25.f };
float camera_defaultzoom[4] = { 0.0f, 0.0f, 0.0f, 0.0f };

// GLOBAL PARAMS END

// Lightweight structure stores parameters to draw a shape.  This will
// vary from app-to-app.
struct RenderItem
{
	RenderItem() = default;
    RenderItem(const RenderItem& rhs) = delete;

    // World matrix of the shape that describes the object's local space
    // relative to the world space, which defines the position, orientation,
    // and scale of the object in the world.
    XMFLOAT4X4 World = MathHelper::Identity4x4();

	XMFLOAT4X4 TexTransform = MathHelper::Identity4x4();

	// Dirty flag indicating the object data has changed and we need to update the constant buffer.
	// Because we have an object cbuffer for each FrameResource, we have to apply the
	// update to each FrameResource.  Thus, when we modify obect data we should set 
	// NumFramesDirty = gNumFrameResources so that each frame resource gets the update.
	int NumFramesDirty = gNumFrameResources;

	// Index into GPU constant buffer corresponding to the ObjectCB for this render item.
	UINT ObjCBIndex = -1;

	Material* Mat = nullptr;
	MeshGeometry* Geo = nullptr;

    // Primitive topology.
    D3D12_PRIMITIVE_TOPOLOGY PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

    // DrawIndexedInstanced parameters.
    UINT IndexCount = 0;
    UINT StartIndexLocation = 0;
    int BaseVertexLocation = 0;
	std::string Name;
};

class TexColumnsApp : public D3DApp
{
public:
    TexColumnsApp(HINSTANCE hInstance);
    TexColumnsApp(const TexColumnsApp& rhs) = delete;
    TexColumnsApp& operator=(const TexColumnsApp& rhs) = delete;
    ~TexColumnsApp();

    virtual bool Initialize()override;

private:
    virtual void OnResize()override;
    virtual void Update(const GameTimer& gt)override;
    virtual void Draw(const GameTimer& gt)override;

    virtual void OnMouseDown(WPARAM btnState, int x, int y)override;
    virtual void OnMouseUp(WPARAM btnState, int x, int y)override;
    virtual void OnMouseMove(WPARAM btnState, int x, int y)override;

	void OnKeyboardInput(const GameTimer& gt);
	void AnimateMaterials(const GameTimer& gt);
	void UpdateObjectCBs(const GameTimer& gt);
	void UpdateMaterialCBs(const GameTimer& gt);
	void UpdateMainPassCB(const GameTimer& gt);
	
	void LoadAllTextures();
	void LoadTexture(const std::string& name);
    void BuildRootSignature();
	void BuildDescriptorHeaps();
    void BuildShadersAndInputLayout();
    void BuildShapeGeometry();
    void BuildPSOs();
	void BuildGBufferAndRenderingSystem();
    void BuildFrameResources();
	void CreateMaterial(std::string _name, int _CBIndex, int _SRVDiffIndex, int _SRVNMapIndex, XMFLOAT4 _DiffuseAlbedo, XMFLOAT3 _FresnelR0, float _Roughness);
    void BuildMaterials();
	void RenderCustomMesh(std::string unique_name, std::string meshname, std::string materialName, XMMATRIX Scale, XMMATRIX Rotation, XMMATRIX Translation);
	void BuildCustomMeshGeometry(std::string name, UINT& meshVertexOffset, UINT& meshIndexOffset, UINT& prevVertSize, UINT& prevIndSize, std::vector<Vertex>& vertices, std::vector<std::uint16_t>& indices, MeshGeometry* Geo);
    void BuildRenderItems();
    void DrawRenderItems(ID3D12GraphicsCommandList* cmdList, const std::vector<RenderItem*>& ritems);

	std::array<const CD3DX12_STATIC_SAMPLER_DESC, 6> GetStaticSamplers();

private:
	std::unordered_map<std::string, unsigned int>ObjectsMeshCount;
    std::vector<std::unique_ptr<FrameResource>> mFrameResources;
    FrameResource* mCurrFrameResource = nullptr;
    int mCurrFrameResourceIndex = 0;
	//
	std::unordered_map<std::string, int>TexOffsets;
	//
    UINT mCbvSrvDescriptorSize = 0;

    ComPtr<ID3D12RootSignature> mRootSignature = nullptr;

	ComPtr<ID3D12DescriptorHeap> mSrvDescriptorHeap = nullptr;

	std::unordered_map<std::string, std::unique_ptr<MeshGeometry>> mGeometries;
	std::unordered_map<std::string, std::unique_ptr<Material>> mMaterials;
	std::unordered_map<std::string, std::unique_ptr<Texture>> mTextures;
	std::unordered_map<std::string, ComPtr<ID3DBlob>> mShaders;
	std::unordered_map<std::string, ComPtr<ID3D12PipelineState>> mPSOs;

	CD3DX12_CPU_DESCRIPTOR_HANDLE mDepthStencilView;

	std::unique_ptr<RenderingSystem> mRenderingSystem;
	std::unique_ptr<GBuffer> mGBuffer; // Добавляем GBuffer

    std::vector<D3D12_INPUT_ELEMENT_DESC> mInputLayout;
 
	// List of all the render items.
	std::vector<std::unique_ptr<RenderItem>> mAllRitems;

	// Render items divided by PSO.
	std::vector<RenderItem*> mOpaqueRitems;

    PassConstants mMainPassCB;
	Camera mCamera;

    float mTheta = 1.5f*XM_PI;
    float mPhi = 0.2f*XM_PI;
    float mRadius = 15.0f;

	float mSunTheta = 1.25f * XM_PI;
	float mSunPhi = XM_PIDIV4;


    POINT mLastMousePos;
};

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE prevInstance,
	PSTR cmdLine, int showCmd)
{
	// Enable run-time memory check for debug builds.
#if defined(DEBUG) | defined(_DEBUG)
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

	try
	{
		TexColumnsApp theApp(hInstance);
		if (!theApp.Initialize())
			return 0;

		return theApp.Run();
	}
	catch (DxException& e)
	{
		MessageBox(nullptr, e.ToString().c_str(), L"HR Failed", MB_OK);
		return 0;
	}
}

TexColumnsApp::TexColumnsApp(HINSTANCE hInstance)
	: D3DApp(hInstance)
{
}

TexColumnsApp::~TexColumnsApp()
{
	if (md3dDevice != nullptr)
		FlushCommandQueue();
}

bool TexColumnsApp::Initialize()
{
	// Создаем консольное окно.
	AllocConsole();

	// Перенаправляем стандартные потоки.
	freopen("CONIN$", "r", stdin);
	freopen("CONOUT$", "w", stdout);
	freopen("CONOUT$", "w", stderr);

	if (!D3DApp::Initialize())
		return false;

	// Reset the command list to prep for initialization commands.
	ThrowIfFailed(mCommandList->Reset(mDirectCmdListAlloc.Get(), nullptr));

	// Get the increment size of a descriptor in this heap type.  This is hardware specific, 
	// so we have to query this information.
	mCbvSrvDescriptorSize = md3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	mCamera.SetPosition(camera_defaultposition[0], camera_defaultposition[1], camera_defaultposition[2]);
	mCamera.RotateY(XMConvertToRadians(180.f));

	mGBuffer = std::make_unique<GBuffer>();
	mRenderingSystem = std::make_unique<RenderingSystem>();
 
	LoadAllTextures();
	std::cout << "Loaded\n";
    BuildRootSignature();
	std::cout << "RS\n";
	BuildDescriptorHeaps();
	std::cout << "DH\n";
    BuildShapeGeometry();
	std::cout << "SG\n";
    BuildShadersAndInputLayout();
	std::cout << "SAIL\n";
	BuildMaterials();
	std::cout << "Mat\n";

	BuildGBufferAndRenderingSystem();
	std::cout << "GBRS\n";

    BuildPSOs();
	std::cout << "PSO\n";

    BuildRenderItems();
	std::cout << "RI\n";
    BuildFrameResources();
	std::cout << "FR\n";

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGui::StyleColorsDark();
	ImGuiIO& io = ImGui::GetIO();
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

	ImGui_ImplDX12_InitInfo init_info = {};
	init_info.Device = md3dDevice.Get();
	init_info.CommandQueue = mCommandQueue.Get();
	init_info.NumFramesInFlight = gNumFrameResources;
	init_info.RTVFormat = DXGI_FORMAT_R8G8B8A8_UNORM; // Or your render target format.
	init_info.DSVFormat = DXGI_FORMAT_UNKNOWN;
	init_info.SrvDescriptorHeap = mSrvDescriptorHeap.Get();
	init_info.LegacySingleSrvCpuDescriptor = mSrvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
	init_info.LegacySingleSrvGpuDescriptor = mSrvDescriptorHeap->GetGPUDescriptorHandleForHeapStart();

	// Execute the initialization commands.
	ThrowIfFailed(mCommandList->Close());
	ID3D12CommandList* cmdsLists[] = { mCommandList.Get() };
	mCommandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

	// Wait until initialization is complete.
	FlushCommandQueue();

	ImGui_ImplWin32_Init(mhMainWnd);
	ImGui_ImplDX12_Init(&init_info);

	std::cout << "initialized!!!" << std::endl;

	return true;
}
 
void TexColumnsApp::OnResize()
{
	D3DApp::OnResize();

	mCamera.SetLens(0.25f * MathHelper::Pi, AspectRatio(), 1.0f, 1000.0f);
}

void TexColumnsApp::OnKeyboardInput(const GameTimer& gt)
{
	const float dt = gt.DeltaTime();

	float camera_speed = 80.0f;
	float camera_zoom = 0.005f;

	if (GetAsyncKeyState('W') & 0x8000)
		mCamera.Walk(camera_speed * dt);

	if (GetAsyncKeyState('S') & 0x8000)
		mCamera.Walk(-camera_speed * dt);

	if (GetAsyncKeyState('A') & 0x8000)
		mCamera.Strafe(-camera_speed * dt);

	if (GetAsyncKeyState('D') & 0x8000)
		mCamera.Strafe(camera_speed * dt);

	if (GetAsyncKeyState('E') & 0x8000)
		mCamera.SetPosition(mCamera.GetPosition3f().x, mCamera.GetPosition3f().y + 0.5f, mCamera.GetPosition3f().z);

	if (GetAsyncKeyState('Q') & 0x8000)
		mCamera.SetPosition(mCamera.GetPosition3f().x, mCamera.GetPosition3f().y - 0.5f, mCamera.GetPosition3f().z);

	if (GetAsyncKeyState('P') & 0x8000)
		mCamera.SetPosition(camera_defaultposition[0], camera_defaultposition[1], camera_defaultposition[2]);

	if (GetAsyncKeyState(VK_LEFT) & 0x8000)
		mSunTheta -= 1.0f * dt;
	if (GetAsyncKeyState(VK_RIGHT) & 0x8000)
		mSunTheta += 1.0f * dt;
	if (GetAsyncKeyState(VK_UP) & 0x8000)
		mSunPhi -= 1.0f * dt;
	if (GetAsyncKeyState(VK_DOWN) & 0x8000)
		mSunPhi += 1.0f * dt;

	mCamera.UpdateViewMatrix();
}

void TexColumnsApp::Update(const GameTimer& gt)
{
	OnKeyboardInput(gt);
    // Cycle through the circular frame resource array.
    mCurrFrameResourceIndex = (mCurrFrameResourceIndex + 1) % gNumFrameResources;
    mCurrFrameResource = mFrameResources[mCurrFrameResourceIndex].get();

    // Has the GPU finished processing the commands of the current frame resource?
    // If not, wait until the GPU has completed commands up to this fence point.
    if(mCurrFrameResource->Fence != 0 && mFence->GetCompletedValue() < mCurrFrameResource->Fence)
    {
        HANDLE eventHandle = CreateEventEx(nullptr, false, false, EVENT_ALL_ACCESS);
        ThrowIfFailed(mFence->SetEventOnCompletion(mCurrFrameResource->Fence, eventHandle));
        WaitForSingleObject(eventHandle, INFINITE);
        CloseHandle(eventHandle);
    }

	AnimateMaterials(gt);
	UpdateObjectCBs(gt);
	UpdateMaterialCBs(gt);
	UpdateMainPassCB(gt);
}

void TexColumnsApp::Draw(const GameTimer& gt)
{
	std::cout << "Draw my niggas\n";
	auto cmdListAlloc = mCurrFrameResource->CmdListAlloc;

	ThrowIfFailed(cmdListAlloc->Reset());
	ThrowIfFailed(mCommandList->Reset(cmdListAlloc.Get(), mPSOs["opaque"].Get()));

	mCommandList->RSSetViewports(1, &mScreenViewport);
	mCommandList->RSSetScissorRects(1, &mScissorRect);

	// Переводим back buffer в режим рендера
	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(
		CurrentBackBuffer(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));

	std::cout << "before rtvhandles size\n";

	auto rtvHandles = mGBuffer->GetRenderTargetViews();

	std::cout << rtvHandles.size() << std::endl;

	if (rtvHandles.empty())
	{
		OutputDebugStringA("Ошибка: rtvHandles пуст, очистка невозможна!\n");
		return;
	}

	if (mDepthStencilView.ptr == 0)
	{
		OutputDebugStringA("Ошибка: mDepthStencilView не инициализирован!\n");
		return;
	}

	mCommandList->OMSetRenderTargets(static_cast<UINT>(rtvHandles.size()), rtvHandles.data(), FALSE, &mDepthStencilView);

	for (int i = 0; i < rtvHandles.size(); i++)
	{
		if (rtvHandles[i].ptr == 0)
		{
			OutputDebugStringA("Ошибка: Один из RTV дескрипторов пуст!\n");
			return;
		}
		mCommandList->ClearRenderTargetView(rtvHandles[i], DirectX::Colors::Black, 0, nullptr);
	}

	mCommandList->ClearDepthStencilView(mDepthStencilView, D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);

	if (!mRenderingSystem)
	{
		OutputDebugStringA("Ошибка: mRenderingSystem не инициализирован!\n");
		return;
	}

	mRenderingSystem->RenderGBuffer(mCommandList.Get(), *mGBuffer);

	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(
		CurrentBackBuffer(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE));

	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(
		CurrentBackBuffer(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET));

	mCommandList->OMSetRenderTargets(1, &CurrentBackBufferView(), true, &DepthStencilView());
	mRenderingSystem->RenderLighting(mCommandList.Get(), *mGBuffer, CurrentBackBuffer());

	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(
		CurrentBackBuffer(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));

	// === ImGui Setup ===
	ImGui_ImplDX12_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();

	ImGui::Begin("Light Settings");

	// Controls for light settings
	ImGui::PushID(0);
	ImGui::Text("Light 1");
	ImGui::SliderFloat3("Position", (float*)&mMainPassCB.Lights[1].Position, -250.f, 250.f);
	ImGui::SliderFloat("Falloff Start", (float*)&mMainPassCB.Lights[1].FalloffStart, 0.f, 100.f);
	ImGui::SliderFloat("Falloff End", (float*)&mMainPassCB.Lights[1].FalloffEnd, 0.f, 100.f);
	ImGui::ColorEdit4("Color", (float*)&mMainPassCB.Lights[1].Color);
	ImGui::PopID();

	ImGui::PushID(1);
	ImGui::Text("Light 2");
	ImGui::SliderFloat3("Direction", (float*)&mMainPassCB.Lights[2].Direction, -250.f, 250.f);
	ImGui::SliderFloat3("Position", (float*)&mMainPassCB.Lights[2].Position, -250.f, 250.f);
	ImGui::SliderFloat("Falloff Start", (float*)&mMainPassCB.Lights[2].FalloffStart, 0.1f, 100.f);
	ImGui::SliderFloat("Falloff End", (float*)&mMainPassCB.Lights[2].FalloffEnd, 0.1f, 100.f);
	ImGui::SliderFloat("Spotlight Power", (float*)&mMainPassCB.Lights[2].SpotPower, 0.1f, 1.f);
	ImGui::ColorEdit4("Color", (float*)&mMainPassCB.Lights[2].Color);
	ImGui::PopID();

	ImGui::PushID(2);
	ImGui::Text("Pig Model");
	int model_id = 1;

	XMFLOAT4X4& worldMatrix = mAllRitems[model_id]->World;

	XMFLOAT3 position(worldMatrix._41, worldMatrix._42, worldMatrix._43);
	XMFLOAT3 scale(
		sqrtf(worldMatrix._11 * worldMatrix._11 + worldMatrix._12 * worldMatrix._12 + worldMatrix._13 * worldMatrix._13),
		sqrtf(worldMatrix._21 * worldMatrix._21 + worldMatrix._22 * worldMatrix._22 + worldMatrix._23 * worldMatrix._23),
		sqrtf(worldMatrix._31 * worldMatrix._31 + worldMatrix._32 * worldMatrix._32 + worldMatrix._33 * worldMatrix._33)
	);

	static XMFLOAT3 rotation = { 0.0f, 0.0f, 0.0f };

	ImGui::DragFloat3("Position", (float*)&position, 0.1f);
	ImGui::DragFloat3("Scale", (float*)&scale, 0.1f, 0.01f, 500.f);
	ImGui::DragFloat3("Rotation", (float*)&rotation, 1.0f, -360.0f, 360.0f);

	// Convert degrees to radians
	XMFLOAT3 rotationRadians = {
		XMConvertToRadians(rotation.x),
		XMConvertToRadians(rotation.y),
		XMConvertToRadians(rotation.z)
	};

	// Recalculate World matrix
	XMMATRIX S = XMMatrixScaling(scale.x, scale.y, scale.z);
	XMMATRIX R = XMMatrixRotationRollPitchYaw(rotationRadians.x, rotationRadians.y, rotationRadians.z);
	XMMATRIX T = XMMatrixTranslation(position.x, position.y, position.z);
	XMMATRIX world = S * R * T;

	XMStoreFloat4x4(&mAllRitems[model_id]->World, world);

	ImGui::PopID();

	ImGui::End();

	// Render the objects
	DrawRenderItems(mCommandList.Get(), mOpaqueRitems);

	ImGui::Render();
	ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), mCommandList.Get());

	// Transition back to present state
	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(
		CurrentBackBuffer(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));

	// Done recording commands.
	ThrowIfFailed(mCommandList->Close());

	// Add the command list to the queue for execution.
	ID3D12CommandList* cmdsLists[] = { mCommandList.Get() };
	mCommandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

	// Swap the back and front buffers
	ThrowIfFailed(mSwapChain->Present(0, 0));
	mCurrBackBuffer = (mCurrBackBuffer + 1) % SwapChainBufferCount;

	// Advance the fence value to mark commands up to this fence point.
	mCurrFrameResource->Fence = ++mCurrentFence;

	// Add an instruction to the command queue to set a new fence point. 
	mCommandQueue->Signal(mFence.Get(), mCurrentFence);
}


void TexColumnsApp::OnMouseDown(WPARAM btnState, int x, int y)
{
    mLastMousePos.x = x;
    mLastMousePos.y = y;

    SetCapture(mhMainWnd);
}

void TexColumnsApp::OnMouseUp(WPARAM btnState, int x, int y)
{
    ReleaseCapture();
}

void TexColumnsApp::OnMouseMove(WPARAM btnState, int x, int y)
{
	if (!ImGui::GetIO().WantCaptureMouse) {
		if ((btnState & MK_LBUTTON) != 0)
		{
			// Make each pixel correspond to a quarter of a degree.
			float dx = XMConvertToRadians(0.25f * static_cast<float>(x - mLastMousePos.x));
			float dy = XMConvertToRadians(0.25f * static_cast<float>(y - mLastMousePos.y));

			// Update angles based on input to orbit camera around box.

			mCamera.Pitch(dy);
			mCamera.RotateY(dx);

		}
		mLastMousePos.x = x;
		mLastMousePos.y = y;
	}
}

void TexColumnsApp::AnimateMaterials(const GameTimer& gt)
{
	
}

void TexColumnsApp::UpdateObjectCBs(const GameTimer& gt)
{
	auto currObjectCB = mCurrFrameResource->ObjectCB.get();
	for(auto& e : mAllRitems)
	{

		// Only update the cbuffer data if the constants have changed.  
		// This needs to be tracked per frame resource.
		if(e->NumFramesDirty > 0)
		{

			orbitAngle += (0.015f * gt.DeltaTime());

			e->NumFramesDirty = gNumFrameResources;

			ObjectConstants objConstants;
			XMMATRIX world = XMLoadFloat4x4(&e->World);
			XMMATRIX texTransform = XMLoadFloat4x4(&e->TexTransform);

			float rotation_angle = 0.45f * gt.TotalTime();
			XMMATRIX tile_rotation = XMMatrixRotationZ(rotation_angle);

			if (e->ObjCBIndex == 0) {
				//texTransform = XMMatrixTranslation(-0.5f, -0.5f, 0.0f) * tile_rotation * XMMatrixTranslation(0.5f, 0.5f, 0.0f) * XMMatrixScaling(100.0f, 100.0f, 1.0f);
				//XMStoreFloat4x4(&e->TexTransform, (XMLoadFloat4x4(&e->TexTransform) * tile_rotation));
			}

			XMStoreFloat4x4(&objConstants.World, XMMatrixTranspose(world));
			XMStoreFloat4x4(&objConstants.TexTransform, XMMatrixTranspose(texTransform));

			currObjectCB->CopyData(e->ObjCBIndex, objConstants);

			// Next FrameResource need to be updated too.
			e->NumFramesDirty--;
		}
	}
}

void TexColumnsApp::UpdateMaterialCBs(const GameTimer& gt)
{
	auto currMaterialCB = mCurrFrameResource->MaterialCB.get();
	for(auto& e : mMaterials)
	{
		Material* mat = e.second.get();
		if(mat->NumFramesDirty > 0)
		{
			XMMATRIX matTransform = XMLoadFloat4x4(&mat->MatTransform);

			MaterialConstants matConstants;
			matConstants.DiffuseAlbedo = mat->DiffuseAlbedo;
			matConstants.FresnelR0 = mat->FresnelR0;
			matConstants.Roughness = mat->Roughness;
			XMStoreFloat4x4(&matConstants.MatTransform, XMMatrixTranspose(matTransform));

			currMaterialCB->CopyData(mat->MatCBIndex, matConstants);

			// Next FrameResource need to be updated too.
			mat->NumFramesDirty--;
		}
	}
}

void TexColumnsApp::UpdateMainPassCB(const GameTimer& gt)
{
	auto currPassCB = mCurrFrameResource->PassCB.get();

	XMMATRIX view = mCamera.GetView();
	XMMATRIX proj = mCamera.GetProj();

	XMMATRIX viewProj = XMMatrixMultiply(view, proj);
	XMMATRIX invView = XMMatrixInverse(&XMMatrixDeterminant(view), view);
	XMMATRIX invProj = XMMatrixInverse(&XMMatrixDeterminant(proj), proj);
	XMMATRIX invViewProj = XMMatrixInverse(&XMMatrixDeterminant(viewProj), viewProj);

	XMVECTOR lightDir = -MathHelper::SphericalToCartesian(1.0f, mSunTheta, mSunPhi);

	XMStoreFloat4x4(&mMainPassCB.View, XMMatrixTranspose(view));
	XMStoreFloat4x4(&mMainPassCB.InvView, XMMatrixTranspose(invView));
	XMStoreFloat4x4(&mMainPassCB.Proj, XMMatrixTranspose(proj));
	XMStoreFloat4x4(&mMainPassCB.InvProj, XMMatrixTranspose(invProj));
	XMStoreFloat4x4(&mMainPassCB.ViewProj, XMMatrixTranspose(viewProj));
	XMStoreFloat4x4(&mMainPassCB.InvViewProj, XMMatrixTranspose(invViewProj));
	mMainPassCB.EyePosW = mCamera.GetPosition3f();
	mMainPassCB.RenderTargetSize = XMFLOAT2((float)mClientWidth, (float)mClientHeight);
	mMainPassCB.InvRenderTargetSize = XMFLOAT2(1.0f / mClientWidth, 1.0f / mClientHeight);
	mMainPassCB.NearZ = 1.0f;
	mMainPassCB.FarZ = 1000.0f;
	mMainPassCB.TotalTime = gt.TotalTime();
	mMainPassCB.DeltaTime = gt.DeltaTime();
	mMainPassCB.AmbientLight = { 0.15f, 0.15f, 0.25f, 1.0f };

	// 0 - dir, 1 - point, 2 - spot

	float dirlight_strength = 0.5f;
	float pointlight_strength = 4.f;
	float spotlight_strength = 3.f;

	XMStoreFloat3(&mMainPassCB.Lights[0].Direction, lightDir);

	//mMainPassCB.Lights[0].Direction = { 0.57735f, -0.57735f, 0.57735f };
	mMainPassCB.Lights[0].Strength = { dirlight_strength, dirlight_strength, dirlight_strength };
	mMainPassCB.Lights[0].Color = {1.0f, 1.0f, 0.0f, 1.0f};
	
	XMVECTOR light1Str = XMLoadFloat3(&mMainPassCB.Lights[1].Strength);
	XMVECTOR light1Pos = XMLoadFloat3(&mMainPassCB.Lights[1].Position);
	float light1FOS = mMainPassCB.Lights[1].FalloffStart;
	float light1FOE = mMainPassCB.Lights[1].FalloffEnd;
	DirectX::XMFLOAT4 light1Color = mMainPassCB.Lights[1].Color;

	mMainPassCB.Lights[1].FalloffStart = light1FOS;
	mMainPassCB.Lights[1].FalloffEnd = light1FOE;
	mMainPassCB.Lights[1].Strength;
	mMainPassCB.Lights[1].Position;
	mMainPassCB.Lights[1].Color = light1Color;

	XMStoreFloat3(&mMainPassCB.Lights[1].Strength, light1Str);
	XMStoreFloat3(&mMainPassCB.Lights[1].Position, light1Pos);

	XMVECTOR light2Dir = XMLoadFloat3(&mMainPassCB.Lights[2].Direction);
	XMVECTOR light2Str = XMLoadFloat3(&mMainPassCB.Lights[2].Strength);
	XMVECTOR light2Pos = XMLoadFloat3(&mMainPassCB.Lights[2].Position);
	float light2FOS = mMainPassCB.Lights[2].FalloffStart;
	float light2FOE = mMainPassCB.Lights[2].FalloffEnd;
	float light2SP = mMainPassCB.Lights[2].SpotPower;
	DirectX::XMFLOAT4 light2Color = mMainPassCB.Lights[2].Color;

	mMainPassCB.Lights[2].FalloffStart = light2FOS;
	mMainPassCB.Lights[2].FalloffEnd = light2FOE;
	mMainPassCB.Lights[2].SpotPower = light2SP;
	mMainPassCB.Lights[2].Direction;
	mMainPassCB.Lights[2].Strength;
	mMainPassCB.Lights[2].Position;
	mMainPassCB.Lights[2].Color = light2Color;

	for (int i = 0; i < lights_needed; i++) {
		float angle = (2.f * XM_PI * i) / lights_needed;
		float x = centr.x + radius * cos(angle);
		float z = centr.y + radius * sin(angle);

		mMainPassCB.Lights[i + 3].FalloffStart = 3.f;
		mMainPassCB.Lights[i + 3].FalloffEnd = 15.f;
		mMainPassCB.Lights[i + 3].SpotPower = 7.5f;
		mMainPassCB.Lights[i + 3].Direction = { 0.f, -0.5, -1.f };
		mMainPassCB.Lights[i + 3].Strength = { spotlight_circle_strength, spotlight_circle_strength, spotlight_circle_strength };
		mMainPassCB.Lights[i + 3].Position = { x, height, z };
		mMainPassCB.Lights[i + 3].Color = {1.0f, 0.0f, 0.0f, 1.0f};

	}

	mMainPassCB.Lights[8].FalloffStart = 3.f;
	mMainPassCB.Lights[8].FalloffEnd = 15.f;
	mMainPassCB.Lights[8].SpotPower = 7.5f;
	mMainPassCB.Lights[8].Direction = { -0.57735f, 1.f, -1.f };
	mMainPassCB.Lights[8].Strength = { spotlight_strength, spotlight_strength, spotlight_strength };
	mMainPassCB.Lights[8].Position = { -10., 10.f, 5.f };
	mMainPassCB.Lights[8].Color = {0.0f, 1.0f, 0.5f, 1.0f};

	for (int i = 3; i < 3 + lights_needed; i++) {
		XMMATRIX rotation = XMMatrixRotationY(orbitAngle);

		XMVECTOR light_pos = XMLoadFloat3(&mMainPassCB.Lights[i].Position);
		light_pos = XMVector3TransformNormal(light_pos, rotation);
		XMStoreFloat3(&mMainPassCB.Lights[i].Position, light_pos);
	}

	currPassCB->CopyData(0, mMainPassCB);
}

void TexColumnsApp::LoadAllTextures()
{
	// MEGA COSTYL
	for (const auto& entry : std::filesystem::directory_iterator("../../Textures/textures"))
	{
		if (entry.is_regular_file() && entry.path().extension() == ".dds")
		{
			std::string filepath = entry.path().string();
			filepath = filepath.substr(24, filepath.size());
			filepath = filepath.substr(0, filepath.size()-4);
			filepath = "textures/" + filepath;
			LoadTexture(filepath);
		}
	}
}

void TexColumnsApp::LoadTexture(const std::string& name)
{
	auto tex = std::make_unique<Texture>();
	tex->Name = name;
	tex->Filename = L"../../Textures/" + std::wstring(name.begin(), name.end()) + L".dds";
	
	if (FAILED(DirectX::CreateDDSTextureFromFile12(md3dDevice.Get(),
		mCommandList.Get(), tex->Filename.c_str(),
		tex->Resource, tex->UploadHeap))) std::cout << name << "\n";
	mTextures[name] = std::move(tex);
}

void TexColumnsApp::BuildRootSignature()
{
	CD3DX12_DESCRIPTOR_RANGE diffuseRange;
	diffuseRange.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0); // Диффузная текстура в регистре t0

	CD3DX12_DESCRIPTOR_RANGE normalRange;
	normalRange.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 1);  // Нормальная карта в регистре t1

    // Root parameter can be a table, root descriptor or root constants.
    CD3DX12_ROOT_PARAMETER slotRootParameter[5];

	// Perfomance TIP: Order from most frequent to least frequent.
	slotRootParameter[0].InitAsDescriptorTable(1, &diffuseRange, D3D12_SHADER_VISIBILITY_PIXEL);
	slotRootParameter[1].InitAsDescriptorTable(1, &normalRange, D3D12_SHADER_VISIBILITY_PIXEL);

    slotRootParameter[2].InitAsConstantBufferView(0); // register b0
    slotRootParameter[3].InitAsConstantBufferView(1); // register b1
    slotRootParameter[4].InitAsConstantBufferView(2); // register b2

	auto staticSamplers = GetStaticSamplers();

    // A root signature is an array of root parameters.
	CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(5, slotRootParameter,
		(UINT)staticSamplers.size(), staticSamplers.data(),
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

    // create a root signature with a single slot which points to a descriptor range consisting of a single constant buffer
    ComPtr<ID3DBlob> serializedRootSig = nullptr;
    ComPtr<ID3DBlob> errorBlob = nullptr;
    HRESULT hr = D3D12SerializeRootSignature(&rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1,
        serializedRootSig.GetAddressOf(), errorBlob.GetAddressOf());

    if(errorBlob != nullptr)
    {
        ::OutputDebugStringA((char*)errorBlob->GetBufferPointer());
    }
    ThrowIfFailed(hr);

    ThrowIfFailed(md3dDevice->CreateRootSignature(
		0,
        serializedRootSig->GetBufferPointer(),
        serializedRootSig->GetBufferSize(),
        IID_PPV_ARGS(mRootSignature.GetAddressOf())));
}
void TexColumnsApp::CreateMaterial(std::string _name, int _CBIndex, int _SRVDiffIndex, int _SRVNMapIndex, XMFLOAT4 _DiffuseAlbedo, XMFLOAT3 _FresnelR0, float _Roughness)
{
	
	auto material = std::make_unique<Material>();
	material->Name = _name;
	material->MatCBIndex = _CBIndex;
	material->DiffuseSrvHeapIndex = _SRVDiffIndex;
	material->NormalSrvHeapIndex = _SRVNMapIndex;
	material->DiffuseAlbedo = _DiffuseAlbedo;
	material->FresnelR0 = _FresnelR0;
	material->Roughness = _Roughness;
	mMaterials[_name] = std::move(material);
}
void TexColumnsApp::BuildDescriptorHeaps()
{
	//
	// Create the SRV heap.
	//
	D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc = {};
	srvHeapDesc.NumDescriptors = mTextures.size();
	srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	ThrowIfFailed(md3dDevice->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(&mSrvDescriptorHeap)));

	D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc = {};
	dsvHeapDesc.NumDescriptors = 1;
	dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
	dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	// ThrowIfFailed(md3dDevice->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(&mDSVHeap)));

	//
	// Fill out the heap with actual descriptors.
	//
	CD3DX12_CPU_DESCRIPTOR_HANDLE hDescriptor(mSrvDescriptorHeap->GetCPUDescriptorHandleForHeapStart());
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	int offset = 0;
	for (const auto& tex : mTextures) {
		auto text = tex.second->Resource;
		srvDesc.Format = text->GetDesc().Format;
		srvDesc.Texture2D.MipLevels = text->GetDesc().MipLevels;
		md3dDevice->CreateShaderResourceView(text.Get(), &srvDesc, hDescriptor);
		hDescriptor.Offset(1, mCbvSrvDescriptorSize);
		TexOffsets[tex.first] = offset;
		offset++;
	}
}

void TexColumnsApp::BuildGBufferAndRenderingSystem()
{
	if (!md3dDevice || !mRtvHeap)
	{
		std::cout << "md3dDevice or mRtvHeap is not initialized!\n";
	}

	std::cout << mRtvDescriptorSize << std::endl;

	mGBuffer->Initialize(
		md3dDevice.Get(),
		mClientWidth,
		mClientHeight
	);

	mRenderingSystem->Initialize(md3dDevice.Get(), mCommandList.Get());
}

void TexColumnsApp::BuildShadersAndInputLayout()
{
	const D3D_SHADER_MACRO alphaTestDefines[] =
	{
		"ALPHA_TEST", "1",
		NULL, NULL
	};

	mShaders["standardVS"] = d3dUtil::CompileShader(L"Shaders\\Default.hlsl", nullptr, "VS", "vs_5_0");
	mShaders["opaquePS"] = d3dUtil::CompileShader(L"Shaders\\Default.hlsl", nullptr, "PS", "ps_5_0");

	mShaders["finalPS"] = d3dUtil::CompileShader(L"Shaders\\finalPS.hlsl", nullptr, "main", "ps_5_0");
	mShaders["gbufferPS"] = d3dUtil::CompileShader(L"Shaders\\gbufferPS.hlsl", nullptr, "main", "ps_5_0");
	mShaders["lightingPS"] = d3dUtil::CompileShader(L"Shaders\\lightingPS.hlsl", nullptr, "main", "ps_5_0");
	mShaders["gbufferVS"] = d3dUtil::CompileShader(L"Shaders\\gbufferVS.hlsl", nullptr, "main", "vs_5_0");
	mShaders["fullscreenVS"] = d3dUtil::CompileShader(L"Shaders\\fullscreenVS.hlsl", nullptr, "main", "vs_5_0");

	mInputLayout =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 32, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "BITANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 44, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	};
}

void TexColumnsApp::BuildCustomMeshGeometry(std::string name, UINT& meshVertexOffset, UINT& meshIndexOffset, UINT& prevVertSize, UINT& prevIndSize, std::vector<Vertex>& vertices, std::vector<std::uint16_t>& indices, MeshGeometry* Geo)
{
	std::vector<GeometryGenerator::MeshData> meshDatas; // Это твоя структура для хранения вершин и индексов

	// Создаем инстанс импортера.
	Assimp::Importer importer;

	// Читаем файл с постпроцессингом: триангуляция, флип UV (если нужно) и генерация нормалей.
	const aiScene* scene = importer.ReadFile("../../Common/" + name + ".obj",
		aiProcess_Triangulate |
		aiProcess_ConvertToLeftHanded |
		aiProcess_FlipUVs |
		aiProcess_GenNormals |
		aiProcess_CalcTangentSpace);
	if (!scene || !scene->mRootNode)
	{
		std::cerr << "Assimp error: " << importer.GetErrorString() << std::endl;
	}
	unsigned int nMeshes = scene->mNumMeshes;
	ObjectsMeshCount[name] = nMeshes;
	
	for (int i = 0;i < scene->mNumMeshes;i++)
	{
		GeometryGenerator::MeshData meshData;
		aiMesh* mesh = scene->mMeshes[i];

		// Подготовка контейнеров для вершин и индексов.
		std::vector<GeometryGenerator::Vertex> vertices;
		std::vector<std::uint16_t> indices;

		// Проходим по всем вершинам и копируем данные.
		for (unsigned int i = 0; i < mesh->mNumVertices; ++i)
		{
			GeometryGenerator::Vertex v;

			v.Position.x = mesh->mVertices[i].x;
			v.Position.y = mesh->mVertices[i].y;
			v.Position.z = mesh->mVertices[i].z;

			if (mesh->HasNormals())
			{
				v.Normal.x = mesh->mNormals[i].x;
				v.Normal.y = mesh->mNormals[i].y;
				v.Normal.z = mesh->mNormals[i].z;
			}

			if (mesh->HasTextureCoords(0))
			{
				v.TexC.x = mesh->mTextureCoords[0][i].x;
				v.TexC.y = mesh->mTextureCoords[0][i].y;
			}
			else
			{
				v.TexC = XMFLOAT2(0.0f, 0.0f);
			}
			if (mesh->HasTangentsAndBitangents())
			{
				v.TangentU.x = mesh->mTangents[i].x;
				v.TangentU.y = mesh->mTangents[i].y;
				v.TangentU.z = mesh->mTangents[i].z;

			}

			// Если необходимо, можно обработать тангенты и другие атрибуты.
			vertices.push_back(v);
		}
		// Проходим по всем граням для формирования индексов.
		for (unsigned int i = 0; i < mesh->mNumFaces; ++i)
		{
			aiFace face = mesh->mFaces[i];
			// Убедимся, что грань треугольная.
			if (face.mNumIndices != 3) continue;
			indices.push_back(static_cast<std::uint16_t>(face.mIndices[0]));
			indices.push_back(static_cast<std::uint16_t>(face.mIndices[1]));
			indices.push_back(static_cast<std::uint16_t>(face.mIndices[2]));
		}

		// Заполняем meshData. Здесь тебе нужно адаптировать под свою структуру:
		meshData.Vertices = vertices;
		meshData.Indices32.resize(indices.size());
		for (size_t j = 0; j < indices.size(); ++j)
			meshData.Indices32[j] = indices[j];

		aiMaterial* mat = scene->mMaterials[mesh->mMaterialIndex];
		aiString texturePath;

		aiString texPath;

		meshData.matName = scene->mMaterials[mesh->mMaterialIndex]->GetName().C_Str();
		// Если требуется, можно выполнить дополнительные операции, например, нормализацию, вычисление тангенсов и т.д.
		meshDatas.push_back(meshData);
	}
	for (int k = 0;k < scene->mNumMaterials;k++)
	{
		aiString texPath;
		scene->mMaterials[k]->GetTexture(aiTextureType_DIFFUSE, 0, &texPath);
		std::string a = std::string(texPath.C_Str());
		a = a.substr(0, a.length() - 4);
		std::cout << "DIFFUSE: " << a << "\n";
		scene->mMaterials[k]->GetTexture(aiTextureType_DISPLACEMENT, 0, &texPath);
		std::string b = std::string(texPath.C_Str());
		b = b.substr(0, b.length() - 4);
		std::cout << "NORMAL: " << b << "\n";

		CreateMaterial(scene->mMaterials[k]->GetName().C_Str(), k, TexOffsets[a], TexOffsets[b], XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f), XMFLOAT3(0.05f, 0.05f, 0.05f), 0.3f);
	}

	UINT totalMeshSize = 0;
	UINT k = vertices.size();
	std::vector<std::pair<GeometryGenerator::MeshData,SubmeshGeometry>>meshSubmeshes;
	for (auto mesh : meshDatas)
	{
		meshVertexOffset = meshVertexOffset + prevVertSize;
		prevVertSize = mesh.Vertices.size();
		totalMeshSize += mesh.Vertices.size();

		meshIndexOffset = meshIndexOffset + prevIndSize;
		prevIndSize = mesh.Indices32.size();
		SubmeshGeometry meshSubmesh;
		meshSubmesh.IndexCount = (UINT)mesh.Indices32.size();
		meshSubmesh.StartIndexLocation = meshIndexOffset;
		meshSubmesh.BaseVertexLocation = meshVertexOffset;
		GeometryGenerator::MeshData m = mesh;
		meshSubmeshes.push_back(std::make_pair(m,meshSubmesh));
	}
	/////////
	/////
	for (auto mesh : meshDatas)
	{
		for (size_t i = 0; i < mesh.Vertices.size(); ++i, ++k)
		{
			vertices.push_back(Vertex(mesh.Vertices[i].Position, mesh.Vertices[i].Normal, mesh.Vertices[i].TexC,mesh.Vertices[i].TangentU));
		}
	}
	////////

	///////
	for (auto mesh : meshDatas)
	{
		indices.insert(indices.end(), std::begin(mesh.GetIndices16()), std::end(mesh.GetIndices16()));
	}
	///////
	Geo->MultiDrawArgs[name] = meshSubmeshes;
}
void TexColumnsApp::BuildShapeGeometry()
{
    GeometryGenerator geoGen;
	GeometryGenerator::MeshData box = geoGen.CreateBox(1.0f, 1.0f, 1.0f, 3);
	GeometryGenerator::MeshData grid = geoGen.CreateGrid(20.0f, 30.0f, 60, 40);
	GeometryGenerator::MeshData sphere = geoGen.CreateSphere(0.5f, 20, 20);
	GeometryGenerator::MeshData cylinder = geoGen.CreateCylinder(0.5f, 0.3f, 3.0f, 20, 20);

	//
	// We are concatenating all the geometry into one big vertex/index buffer.  So
	// define the regions in the buffer each submesh covers.
	//

	// Cache the vertex offsets to each object in the concatenated vertex buffer.
	UINT boxVertexOffset = 0;
	UINT gridVertexOffset = (UINT)box.Vertices.size();
	UINT sphereVertexOffset = gridVertexOffset + (UINT)grid.Vertices.size();
	UINT cylinderVertexOffset = sphereVertexOffset + (UINT)sphere.Vertices.size();

	// Cache the starting index for each object in the concatenated index buffer.
	UINT boxIndexOffset = 0;
	UINT gridIndexOffset = (UINT)box.Indices32.size();
	UINT sphereIndexOffset = gridIndexOffset + (UINT)grid.Indices32.size();
	UINT cylinderIndexOffset = sphereIndexOffset + (UINT)sphere.Indices32.size();
	SubmeshGeometry boxSubmesh;
	boxSubmesh.IndexCount = (UINT)box.Indices32.size();
	boxSubmesh.StartIndexLocation = boxIndexOffset;
	boxSubmesh.BaseVertexLocation = boxVertexOffset;

	SubmeshGeometry gridSubmesh;
	gridSubmesh.IndexCount = (UINT)grid.Indices32.size();
	gridSubmesh.StartIndexLocation = gridIndexOffset;
	gridSubmesh.BaseVertexLocation = gridVertexOffset;

	SubmeshGeometry sphereSubmesh;
	sphereSubmesh.IndexCount = (UINT)sphere.Indices32.size();
	sphereSubmesh.StartIndexLocation = sphereIndexOffset;
	sphereSubmesh.BaseVertexLocation = sphereVertexOffset;

	SubmeshGeometry cylinderSubmesh;
	cylinderSubmesh.IndexCount = (UINT)cylinder.Indices32.size();
	cylinderSubmesh.StartIndexLocation = cylinderIndexOffset;
	cylinderSubmesh.BaseVertexLocation = cylinderVertexOffset;

	//
	// Extract the vertex elements we are interested in and pack the
	// vertices of all the meshes into one vertex buffer.
	//
	
	auto totalVertexCount =
		box.Vertices.size() +
		grid.Vertices.size() +
		sphere.Vertices.size() +
		cylinder.Vertices.size();

	
	std::vector<Vertex> vertices(totalVertexCount);

	UINT k = 0;
	for(size_t i = 0; i < box.Vertices.size(); ++i, ++k)
	{
		vertices[k].Pos = box.Vertices[i].Position;
		vertices[k].Normal = box.Vertices[i].Normal;
		vertices[k].TexC = box.Vertices[i].TexC;
	}

	for(size_t i = 0; i < grid.Vertices.size(); ++i, ++k)
	{
		vertices[k].Pos = grid.Vertices[i].Position;
		vertices[k].Normal = grid.Vertices[i].Normal;
		vertices[k].TexC = grid.Vertices[i].TexC;
	}

	for(size_t i = 0; i < sphere.Vertices.size(); ++i, ++k)
	{
		vertices[k].Pos = sphere.Vertices[i].Position;
		vertices[k].Normal = sphere.Vertices[i].Normal;
		vertices[k].TexC = sphere.Vertices[i].TexC;
	}

	for(size_t i = 0; i < cylinder.Vertices.size(); ++i, ++k)
	{
		vertices[k].Pos = cylinder.Vertices[i].Position;
		vertices[k].Normal = cylinder.Vertices[i].Normal;
		vertices[k].TexC = cylinder.Vertices[i].TexC;
	}
	
	std::vector<std::uint16_t> indices;
	indices.insert(indices.end(), std::begin(box.GetIndices16()), std::end(box.GetIndices16()));
	indices.insert(indices.end(), std::begin(grid.GetIndices16()), std::end(grid.GetIndices16()));
	indices.insert(indices.end(), std::begin(sphere.GetIndices16()), std::end(sphere.GetIndices16()));
	indices.insert(indices.end(), std::begin(cylinder.GetIndices16()), std::end(cylinder.GetIndices16()));
	
	
	
	UINT meshVertexOffset = cylinderVertexOffset;
	UINT meshIndexOffset = cylinderIndexOffset;
	UINT prevIndSize = (UINT)cylinder.Indices32.size();
	UINT prevVertSize = (UINT)cylinder.Vertices.size();

	auto geo = std::make_unique<MeshGeometry>();
	geo->Name = "shapeGeo";
	BuildCustomMeshGeometry("minecraft_pig", meshVertexOffset, meshIndexOffset, prevVertSize, prevIndSize, vertices, indices, geo.get());
	BuildCustomMeshGeometry("sponza", meshVertexOffset, meshIndexOffset, prevVertSize, prevIndSize, vertices, indices, geo.get());
	BuildCustomMeshGeometry("negr", meshVertexOffset, meshIndexOffset, prevVertSize, prevIndSize, vertices, indices, geo.get());
	BuildCustomMeshGeometry("left", meshVertexOffset, meshIndexOffset, prevVertSize, prevIndSize, vertices, indices, geo.get());
	BuildCustomMeshGeometry("right", meshVertexOffset, meshIndexOffset, prevVertSize, prevIndSize, vertices, indices, geo.get());
	



	const UINT vbByteSize = (UINT)vertices.size() * sizeof(Vertex);
	const UINT ibByteSize = (UINT)indices.size() * sizeof(std::uint16_t);




	ThrowIfFailed(D3DCreateBlob(vbByteSize, &geo->VertexBufferCPU));
	CopyMemory(geo->VertexBufferCPU->GetBufferPointer(), vertices.data(), vbByteSize);

	ThrowIfFailed(D3DCreateBlob(ibByteSize, &geo->IndexBufferCPU));
	CopyMemory(geo->IndexBufferCPU->GetBufferPointer(), indices.data(), ibByteSize);

	geo->VertexBufferGPU = d3dUtil::CreateDefaultBuffer(md3dDevice.Get(),
		mCommandList.Get(), vertices.data(), vbByteSize, geo->VertexBufferUploader);

	geo->IndexBufferGPU = d3dUtil::CreateDefaultBuffer(md3dDevice.Get(),
		mCommandList.Get(), indices.data(), ibByteSize, geo->IndexBufferUploader);

	geo->VertexByteStride = sizeof(Vertex);
	geo->VertexBufferByteSize = vbByteSize;
	geo->IndexFormat = DXGI_FORMAT_R16_UINT;
	geo->IndexBufferByteSize = ibByteSize;

	geo->DrawArgs["box"] = boxSubmesh;
	geo->DrawArgs["grid"] = gridSubmesh;
	geo->DrawArgs["sphere"] = sphereSubmesh;
	geo->DrawArgs["cylinder"] = cylinderSubmesh;

	mGeometries[geo->Name] = std::move(geo);
}

void TexColumnsApp::BuildPSOs()
{
	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc;
	ZeroMemory(&psoDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));

	// PSO for GBuffer Pass
	psoDesc.InputLayout = { mInputLayout.data(), (UINT)mInputLayout.size() };
	psoDesc.pRootSignature = mRootSignature.Get();
	psoDesc.VS = { reinterpret_cast<BYTE*>(mShaders["gbufferVS"]->GetBufferPointer()), mShaders["gbufferVS"]->GetBufferSize() };
	psoDesc.PS = { reinterpret_cast<BYTE*>(mShaders["gbufferPS"]->GetBufferPointer()), mShaders["gbufferPS"]->GetBufferSize() };
	psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	psoDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
	psoDesc.SampleMask = UINT_MAX;
	psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

	psoDesc.NumRenderTargets = mGBuffer->GetRenderTargetCount();
	for (UINT i = 0; i < psoDesc.NumRenderTargets; ++i)
	{
		psoDesc.RTVFormats[i] = mGBuffer->GetFormat(i);
	}
	psoDesc.SampleDesc.Count = 1;
	psoDesc.DSVFormat = mDepthStencilFormat;
	ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&mPSOs["gbuffer"])));
}


void TexColumnsApp::BuildFrameResources()
{
	FlushCommandQueue();
	mFrameResources.clear();
    for(int i = 0; i < gNumFrameResources; ++i)
    {
        mFrameResources.push_back(std::make_unique<FrameResource>(md3dDevice.Get(),
            1, (UINT)mAllRitems.size(), (UINT)mMaterials.size()));
    }
	mCurrFrameResourceIndex = 0;
	mCurrFrameResource = mFrameResources[mCurrFrameResourceIndex].get();
	for (auto& ri : mAllRitems)
	{
		ri->NumFramesDirty = gNumFrameResources;
	}
	for (auto& kv : mMaterials)
	{
		kv.second->NumFramesDirty = gNumFrameResources;
	}
}

void TexColumnsApp::BuildMaterials()
{
	CreateMaterial("pig", 0, TexOffsets["textures/pig"], TexOffsets["textures/pig"], XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f), XMFLOAT3(0.05f, 0.05f, 0.05f), 0.3f);
	CreateMaterial("well_in_that_case", 0, TexOffsets["textures/well_in_that_case"], NULL, XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f), XMFLOAT3(0.05f, 0.05f, 0.05f), 0.3f);
	CreateMaterial("NiggaMat",0, TexOffsets["textures/texture"], TexOffsets["textures/texture_nm"], XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f), XMFLOAT3(0.05f, 0.05f, 0.05f), 0.3f);
	CreateMaterial("eye",0, TexOffsets["textures/eye"], TexOffsets["textures/eye_nm"], XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f), XMFLOAT3(0.05f, 0.05f, 0.05f), 0.3f);
}
void TexColumnsApp::RenderCustomMesh(std::string unique_name, std::string meshname, std::string materialName, XMMATRIX Scale, XMMATRIX Rotation, XMMATRIX Translation)
{
	for (int i = 0;i < ObjectsMeshCount[meshname];i++)
	{
		auto rItem = std::make_unique<RenderItem>();
		std::string textureFile;
		rItem->Name = unique_name;
		XMStoreFloat4x4(&rItem->TexTransform, XMMatrixScaling(1., 1., 1.));
		XMStoreFloat4x4(&rItem->World, Scale * Rotation * Translation);
		rItem->ObjCBIndex = mAllRitems.size();
		rItem->Geo = mGeometries["shapeGeo"].get();
		rItem->PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
		std::string matname = rItem->Geo->MultiDrawArgs[meshname][i].first.matName;
		std::cout << " mat : " << matname << "\n";
		std::cout << unique_name << " " << matname << "\n";
		if (materialName != "") matname = materialName;
		rItem->Mat = mMaterials[matname].get();
		rItem->IndexCount = rItem->Geo->MultiDrawArgs[meshname][i].second.IndexCount;
		rItem->StartIndexLocation = rItem->Geo->MultiDrawArgs[meshname][i].second.StartIndexLocation;
		rItem->BaseVertexLocation = rItem->Geo->MultiDrawArgs[meshname][i].second.BaseVertexLocation;
		mAllRitems.push_back(std::move(rItem));
		mOpaqueRitems.push_back(mAllRitems[mAllRitems.size() - 1].get());
	}
	BuildFrameResources();
}



void TexColumnsApp::BuildRenderItems()
{
	auto grass_plane = std::make_unique<RenderItem>();
	XMMATRIX grass_plane_position = XMMatrixScaling(500.f, 500.0f, 0.1f) * XMMatrixRotationY(XMConvertToRadians(90.f)) * XMMatrixTranslation(200.f, 10.f, 40.f);

	XMStoreFloat4x4(&grass_plane->World, grass_plane_position);
	XMStoreFloat4x4(&grass_plane->TexTransform, XMMatrixScaling(100.0f, 100.0f, 1.0f));
	grass_plane->ObjCBIndex = 0;
	grass_plane->Mat = mMaterials["well_in_that_case"].get();
	grass_plane->Geo = mGeometries["shapeGeo"].get();
	grass_plane->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	grass_plane->IndexCount = grass_plane->Geo->DrawArgs["box"].IndexCount;
	grass_plane->StartIndexLocation = grass_plane->Geo->DrawArgs["box"].StartIndexLocation;
	grass_plane->BaseVertexLocation = grass_plane->Geo->DrawArgs["box"].BaseVertexLocation;

	mAllRitems.push_back(std::move(grass_plane));

	RenderCustomMesh("piggy", "minecraft_pig", "pig", XMMatrixScaling(15.f, 15.f, 15.f), XMMatrixRotationRollPitchYaw(0, 3.14, 0), XMMatrixTranslation(0, 10, 0));

	RenderCustomMesh("building", "sponza", "", XMMatrixScaling(0.07, 0.07, 0.07), XMMatrixRotationRollPitchYaw(0, 3.14 / 2, 0), XMMatrixTranslation(0, 0, 0));
	// All the render items are opaque.
	for(auto& e : mAllRitems)
		mOpaqueRitems.push_back(e.get());
}

void TexColumnsApp::DrawRenderItems(ID3D12GraphicsCommandList* cmdList, const std::vector<RenderItem*>& ritems)
{
    UINT objCBByteSize = d3dUtil::CalcConstantBufferByteSize(sizeof(ObjectConstants));
    UINT matCBByteSize = d3dUtil::CalcConstantBufferByteSize(sizeof(MaterialConstants));
 
	auto objectCB = mCurrFrameResource->ObjectCB->Resource();
	auto matCB = mCurrFrameResource->MaterialCB->Resource();

    // For each render item...
    for(size_t i = 0; i < ritems.size(); ++i)
    {
        auto ri = ritems[i];
        cmdList->IASetVertexBuffers(0, 1, &ri->Geo->VertexBufferView());
        cmdList->IASetIndexBuffer(&ri->Geo->IndexBufferView());
        cmdList->IASetPrimitiveTopology(ri->PrimitiveType);

		CD3DX12_GPU_DESCRIPTOR_HANDLE diffuseHandle(mSrvDescriptorHeap->GetGPUDescriptorHandleForHeapStart());
		diffuseHandle.Offset(ri->Mat->DiffuseSrvHeapIndex, mCbvSrvDescriptorSize);
		cmdList->SetGraphicsRootDescriptorTable(0, diffuseHandle);
		CD3DX12_GPU_DESCRIPTOR_HANDLE normalHandle(mSrvDescriptorHeap->GetGPUDescriptorHandleForHeapStart());
		normalHandle.Offset(ri->Mat->NormalSrvHeapIndex, mCbvSrvDescriptorSize);
		cmdList->SetGraphicsRootDescriptorTable(1, normalHandle);

		//// Получаем дескриптор для нормальной карты по её оффсету.
		//CD3DX12_GPU_DESCRIPTOR_HANDLE normalHandle(mSrvDescriptorHeap->GetGPUDescriptorHandleForHeapStart());
		//normalHandle.Offset(ri->Mat->NormalSrvHeapIndex, mCbvSrvDescriptorSize);
		//cmdList->SetGraphicsRootDescriptorTable(1, normalHandle);

        D3D12_GPU_VIRTUAL_ADDRESS objCBAddress = objectCB->GetGPUVirtualAddress() + ri->ObjCBIndex*objCBByteSize;
		D3D12_GPU_VIRTUAL_ADDRESS matCBAddress = matCB->GetGPUVirtualAddress() + ri->Mat->MatCBIndex*matCBByteSize;

        cmdList->SetGraphicsRootConstantBufferView(2, objCBAddress);
        cmdList->SetGraphicsRootConstantBufferView(4, matCBAddress);

        cmdList->DrawIndexedInstanced(ri->IndexCount, 1, ri->StartIndexLocation, ri->BaseVertexLocation, 0);
    }
}

std::array<const CD3DX12_STATIC_SAMPLER_DESC, 6> TexColumnsApp::GetStaticSamplers()
{
	// Applications usually only need a handful of samplers.  So just define them all up front
	// and keep them available as part of the root signature.  

	const CD3DX12_STATIC_SAMPLER_DESC pointWrap(
		0, // shaderRegister
		D3D12_FILTER_MIN_MAG_MIP_POINT, // filter
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressU
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressV
		D3D12_TEXTURE_ADDRESS_MODE_WRAP); // addressW

	const CD3DX12_STATIC_SAMPLER_DESC pointClamp(
		1, // shaderRegister
		D3D12_FILTER_MIN_MAG_MIP_POINT, // filter
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressU
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressV
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP); // addressW

	const CD3DX12_STATIC_SAMPLER_DESC linearWrap(
		2, // shaderRegister
		D3D12_FILTER_MIN_MAG_MIP_LINEAR, // filter
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressU
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressV
		D3D12_TEXTURE_ADDRESS_MODE_WRAP); // addressW

	const CD3DX12_STATIC_SAMPLER_DESC linearClamp(
		3, // shaderRegister
		D3D12_FILTER_MIN_MAG_MIP_LINEAR, // filter
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressU
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressV
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP); // addressW

	const CD3DX12_STATIC_SAMPLER_DESC anisotropicWrap(
		4, // shaderRegister
		D3D12_FILTER_ANISOTROPIC, // filter
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressU
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressV
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressW
		0.0f,                             // mipLODBias
		8);                               // maxAnisotropy

	const CD3DX12_STATIC_SAMPLER_DESC anisotropicClamp(
		5, // shaderRegister
		D3D12_FILTER_ANISOTROPIC, // filter
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressU
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressV
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressW
		0.0f,                              // mipLODBias
		8);                                // maxAnisotropy

	return { 
		pointWrap, pointClamp,
		linearWrap, linearClamp, 
		anisotropicWrap, anisotropicClamp };
}

