// project.Timestamp 15.09.2020-25.01.2021 | Barracuda | Mixed launch x64 
//Debug injection support.1-load driver. 2-start debug
#include "definitions.h"
#include "ioctls.h"

//#include "security.h"

#define drv_dos_device L"\\DosDevices\\FreqOml"
#define drv_device L"\\Device\\FreqOml"
#define drv  L"\\Driver\\FreqOml"

catch (std::exception& ex) {
		MessageBoxA(NULL, ex.what(), "Exception", 0);
		return;
	}
}


void GetProcess("Rust.exe")
	
	GetWindowThreadProcessId(Globals::tWnd, &procID);
	Globals::hGame = OpenProcess(PROCESS_ALL_ACCESS, FALSE, procID);

	Globals::hWnd = InitializeWin((HINSTANCE)hInst);
	MSG uMessage;

	if (Globals::hWnd == NULL) { exit(1); }

	ShowWindow(Globals::hWnd, SW_SHOW);

	INITIALIZED = TRUE;

	while (!UNLOADING) {
		if (PeekMessage(&uMessage, Globals::hWnd, 0, 0, PM_REMOVE)) {
			DispatchMessage(&uMessage);
			TranslateMessage(&uMessage);
		}

		if (UNLOADING) {
			HWND hMsg = FindWindow(NULL, "Info");

			if (hMsg) {
				std::this_thread::sleep_for(std::chrono::seconds(3));
				SendMessageA(hMsg, WM_CLOSE, 0, 0);
			}
		}
	}




NTSTATUS
{
	NTSTATUS status;
	PIRP irp;
	PIO_STACK_LOCATION irp::IoStackLocation:
		irp = IoAllocateTrp(device, irp);
		if (!irp)
			return STATUS_INSUFFICIENT_RESOURCES;
			IoSetNextIrpStackLocation(irp);
			status = IoCallDriver(device, irp);
			return status
}

//static NTSTATUS ioctl_close(PDEVICE_OBJECT device, PIRP irp) {
	// irp->IoStatus.Status = STATUS_SUCCESS;

}

NTSTATUS ioctl_close(PDEVICE_OBJECT device, PIRP irp);
NTSTATUS io_device_control(PDEVICE_OBJECT device, PIRP Irp);
__inline NTSTATUS copy_memory(PEPROCESS src_proc, PEPROCESS target_proc, PVOID src, PVOID dst, SIZE_T size);
NTSTATUS ioctl_create(PDEVICE_OBJECT device, PIRP irp);

ULONGLONG get_module_handle(ULONG pid, LPCWSTR module_name);

NTSTATUS init(PDRIVER_OBJECT driver, PUNICODE_STRING path) {
	
	RtlInitUnicodeString(&dev, drv_device);
	RtlInitUnicodeString(&dos, drv_dos_device);


	IoCreateDevice(driver, 0, &dev, FILE_DEVICE_UNKNOWN, FILE_DEVICE_SECURE_OPEN, FALSE, &device_object);
	IoCreateSymbolicLink(&dos, &dev);

	driver->MajorFunction[IRP_MJ_CLOSE] = ioctl_close;
	driver->MajorFunction[IRP_MJ_DEVICE_CONTROL] = io_device_control;
	driver->MajorFunction[IRP_MJ_CREATE] = ioctl_create;

	//	driver->DriverUnload = unload_driver;

	device_object->Flags |= DO_DIRECT_IO;
	device_object->Flags &= ~DO_DEVICE_INITIALIZING;

	//DbgPrint("[DRIVER] DriverIntit");


	return STATUS_SUCCESS;
}

NTSTATUS DriverEntry(PDRIVER_OBJECT driver, PUNICODE_STRING path) 
{
	
	/*
	UNICODE_STRING driver_name = RTL_CONSTANT_STRING(L"iqvw64e.sys");//Capcom.sys 0x57cd1415 (timeDateStamp)
	clearCache(driver_name, 0x5284EAC3);
	if (!FindMmDriverData())
	{
		if (ClearUnloadedDriver(&driver_name, true) == STATUS_SUCCESS)true;//DbgPrint("ClearUnloadedDriver sucessful");
		//else DbgPrint("ClearUnloadedDriver error");	
	}
	else
	{
		//DbgPrint("MmDriverData not find");
	}
	*/

	NTSTATUS        status;
	UNICODE_STRING drv_name;
	RtlInitUnicodeString(&drv_name, drv);
	return IoCreateDriver(&drv_name, &init);

}

NTSTATUS io_device_control(PDEVICE_OBJECT device, PIRP irp) {
	NTSTATUS status;
	ULONG info_size = 0;
	PIO_STACK_LOCATION stack = IoGetCurrentIrpStackLocation(irp);
	ULONG control_code = stack->Parameters.DeviceIoControl.IoControlCode;

	switch (control_code) {


	case ioctl_allocate_virtual_memory: {
		//	DbgPrint("[DRIVER] ioctl_allocate_virtual_memory");
		pk_alloc_mem_request in = (pk_alloc_mem_request)irp->AssociatedIrp.SystemBuffer;
		PEPROCESS target_proc;
		status = PsLookupProcessByProcessId((HANDLE)in->pid, &target_proc);
		if (NT_SUCCESS(status)) {
			KAPC_STATE apc;
			KeStackAttachProcess(target_proc, &apc);
			status = ZwAllocateVirtualMemory(ZwCurrentProcess(), (PVOID*)&in->addr, 0, &in->size,
				in->allocation_type, in->protect);
			KeUnstackDetachProcess(&apc);
			ObfDereferenceObject(target_proc);
		}
		info_size = sizeof(k_alloc_mem_request);
	} break;

	case ioctl_protect_virutal_memory: {

		pk_protect_mem_request in = (pk_protect_mem_request)irp->AssociatedIrp.SystemBuffer;
		PEPROCESS target_proc;
		status = PsLookupProcessByProcessId((HANDLE)in->pid, &target_proc);
		if (NT_SUCCESS(status)) {
			KAPC_STATE apc;
			ULONG old_protection;
			KeStackAttachProcess(target_proc, &apc);
			status = ZwProtectVirtualMemory(ZwCurrentProcess(), (PVOID*)&in->addr, &in->size, in->protect, &old_protection);
			KeUnstackDetachProcess(&apc);
			in->protect = old_protection;
			ObfDereferenceObject(target_proc);
		}
		info_size = sizeof(k_protect_mem_request);
	} break;

	case ioctl_copy_memory: {
		pk_rw_request in = (pk_rw_request)irp->AssociatedIrp.SystemBuffer;
		PEPROCESS src_proc;
		PEPROCESS dst_proc;
		status = PsLookupProcessByProcessId((HANDLE)in->src_pid, &src_proc);
		status |= PsLookupProcessByProcessId((HANDLE)in->dst_pid, &dst_proc);
		if (NT_SUCCESS(status)) {
			status = copy_memory(src_proc, dst_proc, (PVOID)in->src_addr, (PVOID)in->dst_addr, in->size);
			ObfDereferenceObject(dst_proc);
			ObfDereferenceObject(src_proc);
			//	if (!NT_SUCCESS(status))DbgPrint("[DRIVER] copy_memory error");
		}
		info_size = sizeof(k_rw_request);
	} break;

	case ioctl_get_module_base: {
		pk_get_base_module_request in = (pk_get_base_module_request)irp->AssociatedIrp.SystemBuffer;
		ULONGLONG handle = get_module_handle(in->pid, in->name);
		in->handle = handle;
		status = STATUS_SUCCESS;
		info_size = sizeof(k_get_base_module_request);
	} break;


	


	default:
		status = STATUS_INVALID_PARAMETER;
		info_size = 0;
		break;
	}


	irp->IoStatus.Status = status;
	irp->IoStatus.Information = info_size;
	IoCompleteRequest(irp, IO_NO_INCREMENT);
	return status;
}

DWORD WINAPI ThreadProc(
	_In_ LPVOID lpParameter
) {
	RunCheat();
	TerminateProcess(GetCurrentProcess(), 0);
	return 0;
}


NTSTATUS ioctl_create(PDEVICE_OBJECT device, PIRP irp) {
	//DbgPrint("[DRIVER] ioctl_CREATE");
	irp->IoStatus.Status = STATUS_SUCCESS;
	irp->IoStatus.Information = 0;
	IoCompleteRequest(irp, IO_NO_INCREMENT);
	return STATUS_SUCCESS;
}

/*
NTSTATUS unload_driver(PDRIVER_OBJECT driver) {
	IoDeleteSymbolicLink(&dos);
	IoDeleteDevice(driver->DeviceObject);
}
*/


ULONGLONG get_module_handle(ULONG pid, LPCWSTR module_name) {
	PEPROCESS target_proc;
	ULONGLONG base = 0;
	if (!NT_SUCCESS(PsLookupProcessByProcessId((HANDLE)pid, &target_proc)))
		return 0;

	KeAttachProcess((PKPROCESS)target_proc);

	PPEB peb = PsGetProcessPeb(target_proc);
	if (!peb)goto end;

	if (!peb->Ldr || !peb->Ldr->Initialized)goto end;

end:
	
}

	KeDetachProcess();
	ObDereferenceObject(target_proc);
	return base;
}

NTSTATUS ioctl_close(PDEVICE_OBJECT device, PIRP irp) {
	irp->IoStatus.Status = STATUS_SUCCESS;
	irp->IoStatus.Information = 0;
	IoCompleteRequest(irp, IO_NO_INCREMENT);
	return STATUS_SUCCESS;
}

__inline NTSTATUS copy_memory(PEPROCESS src_proc, PEPROCESS target_proc, PVOID src, PVOID dst, SIZE_T size) {
	SIZE_T bytes;
	return MmCopyVirtualMemory(src_proc, src, target_proc, dst, size, UserMode, &bytes);
}

void std::private_create_logger()
{
	g_log = std::make_unique<logger>();
}

{
	setting aimbot smooth("aimbot", "smooth",0.5f); 
}

long __stdcall DllMain(void* mod, uint32_t reason, void* reserved) {
    switch (reason) {
    case DLL_PROCESS_ATTACH:
        CreateThread(nullptr, 0, (LPTHREAD_START_ROUTINE)main, mod, 0, nullptr);
        break;
    }

    return 1;
}


int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {

	CreateThread(0, 0, (LPTHREAD_START_ROUTINE)Cheat::Update, 0, 0, 0);
	CreateThread(0, 0, (LPTHREAD_START_ROUTINE)UpdateWinPosition, 0, 0, 0);

	while (Globals::rWidth < 640 && Globals::rHeight < 480) {
		Globals::tWnd = FindWindow(NULL, "Rust");

		RECT wSize;
		GetWindowRect(Globals::tWnd, &wSize);
		Globals::rWidth = wSize.right - wSize.left;
		Globals::rHeight = wSize.bottom - wSize.top;
	}
