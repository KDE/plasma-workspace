#!/usr/bin/env python3

# SPDX-FileCopyrightText: 2025 Bohdan Onofriichuk <bogdan.onofriuchuk@gmail.com>
# SPDX-License-Identifier: MIT

"""udisks2 mock template

This creates the expected methods and properties of the main
UDisks2 object and the manager, and adds methods to manipulate a simple
mock drive.
"""

import dbus

from datetime import datetime

import dbusmock
from dbusmock import MOCK_IFACE

SYSTEM_BUS = True
IS_OBJECT_MANAGER = True

BUS_NAME = "org.freedesktop.UDisks2"
MAIN_OBJ = "/org/freedesktop/UDisks2"
MAIN_IFACE = 'org.freedesktop.UDisks2'

UDISKS2_MANAGER_IFACE = "org.freedesktop.UDisks2.Manager"
UDISKS2_MANAGER_OBJ = "/org/freedesktop/UDisks2/Manager"

UDISKS2_BLOCK_DEVICES = "/org/freedesktop/UDisks2/block_devices/"
UDISKS2_DRIVE_DEVICES = "/org/freedesktop/UDisks2/drives/"
UDISKS2_JOBS = "/org/freedesktop/UDisks2/jobs/"

UDISKS2_JOB_IFACE = "org.freedesktop.UDisks2.Job"
UDISKS2_DRIVE_IFACE = "org.freedesktop.UDisks2.Drive"
UDISKS2_BLOCK_IFACE = "org.freedesktop.UDisks2.Block"
UDISKS2_PARTITION_IFACE = "org.freedesktop.UDisks2.Partition"
UDISKS2_FILESYSTEM_IFACE = "org.freedesktop.UDisks2.Filesystem"

MOUNT_POINT = "/run/media/"

def load(mock, parameters):

    supported_filesystems = ["ext4"]

    manager_props = {
        "Version": dbus.String("2.10.2"),
        "SupportedFilesystems": dbus.Array(supported_filesystems, signature="s"),
        "SupportedEncryptionTypes": dbus.Array([], signature="s"),
        "DefaultEncryptionTypes": dbus.Array([], signature="s"),
    }

    manager_methods = [
        ("CanFormat", "s", "(bs)", "ret = self.can_format(args[0]), \"\""),
        ("CanResize", "s", "(bts)", "ret = self.can_resize(args[0]), \"\""),
        ("CanCheck", "s", "(bs)", "ret = self.can_check(args[0]), \"\""),
        ("CanRepair", "s", "(bs)", "ret = self.can_repair(args[0]), \"\""),
        ("GetBlockDevices", "a{sv}", "ao", "ret = self.get_block_paths()"),
        ("GetDrives", "a{sv}", "ao", "ret = self.get_drive_paths()"),
    ]

    mock.AddObject(UDISKS2_MANAGER_OBJ, UDISKS2_MANAGER_IFACE, manager_props, manager_methods)

    obj = dbusmock.get_object(UDISKS2_MANAGER_OBJ)
    obj.block_devices = []
    obj.drive_devices = []
    obj.dev_t = 0
    obj.job_counter = 0
    obj.supported_filesystems = supported_filesystems

@dbus.service.method(MOCK_IFACE,
                     in_signature="sttb", out_signature="")
def add_mock_device(self, device_name, device_number, device_size, is_damaged):
    """
    Convenience method to add a mock device
    """

    block_full_name = device_name + str(device_number)
    drive_full_name = "MOCK_FLASH_" + block_full_name

    drive_props = {
        "Vendor": dbus.String("MockDevice"),
        "Model": dbus.String("MockModel"),
        "Revision": dbus.String("MockRevision"),
        "Serial": dbus.String("MockSerial"),
        "WWN": dbus.String("MockWWN"),
        "Id": dbus.String("by-mock-" + str(device_number)),
        "Configuration": dbus.Array([], signature="(sa{sv})"),
        "Media": dbus.String(""),
        "MediaCompatibility": dbus.Array([], signature="s"),
        "MediaRemovable": dbus.Boolean(True),
        "MediaAvailable": dbus.Boolean(True),
        "MediaChangeDetected": dbus.Boolean(True),
        "Size": dbus.UInt64(device_size),
        "TimeDetected": dbus.UInt64(0),
        "TimeMediaDetected": dbus.UInt64(0),
        "Optical": dbus.Boolean(False),
        "OpticalBlank": dbus.Boolean(False),
        "OpticalNumTracks": dbus.UInt32(0),
        "OpticalNumAudioTracks": dbus.UInt32(0),
        "OpticalNumDataTracks": dbus.UInt32(0),
        "OpticalNumSessions": dbus.UInt32(0),
        "RotationRate": dbus.Int32(-1),
        "ConnectionBus": dbus.String("usb"),
        "Seat": dbus.String("seat0"),
        "Removable": dbus.Boolean(True),
        "Ejectable": dbus.Boolean(True),
        "SortKey": dbus.String("mock"),
        "CanPowerOff": dbus.Boolean(True),
        "SiblingId": dbus.String("mock")
    }

    drive_methods = [
        ("Eject", "a{sv}", "", "self.eject_drive(self.drive_name)"),
        ("SetConfiguration", "a{sv}a{sv}", "", "pass"),
        ("PowerOff", "a{sv}", "", "pass"),
    ]

    mng_obj = dbusmock.get_object(UDISKS2_MANAGER_OBJ)

    block_props = {
        "Device": dbus.ByteArray((MOUNT_POINT + block_full_name).encode("ascii")),
        "PreferredDevice": dbus.ByteArray((MOUNT_POINT + block_full_name).encode("ascii")),
        "Symlinks": dbus.Array([], signature="ay"),
        "DeviceNumber": dbus.UInt64(mng_obj.dev_t),
        "Id": dbus.String("by-mock-" + block_full_name),
        "Size": dbus.UInt64(device_size),
        "ReadOnly": dbus.Boolean(False),
        "Drive": dbus.ObjectPath(UDISKS2_DRIVE_DEVICES + drive_full_name),
        "MDRaid": dbus.ObjectPath("/"),
        "MDRaidMember": dbus.ObjectPath("/"),
        "IdUsage": dbus.String("filesystem"),
        "IdType": dbus.String("ext4"),
        "IdVersion": dbus.String("1.0"),
        "IdLabel": dbus.String(block_full_name),
        "IdUUID": dbus.String("86bf4da8-af29-406a-86f6-8eb9e11c99bd"),
        "Configuration": dbus.Array([], signature="(sa{sv})"),
        "CryptoBackingDevice": dbus.ObjectPath("/"),
        "HintPartitionable": dbus.Boolean(True),
        "HintSystem": dbus.Boolean(False),
        "HintIgnore": dbus.Boolean(False),
        "HintAuto": dbus.Boolean(False),
        "HintName": dbus.String(""),
        "HintIconName": dbus.String(""),
        "HintSymbolicIconName": dbus.String(""),
        "UserspaceMountOptions": dbus.Array([], signature="s"),
    }

    mng_obj.dev_t += 1

    block_methods = [
        ("AddConfigurationItem", "(sa{sv})a{sv}", "", "pass"),
        ("RemoveConfigurationItem", "(sa{sv})a{sv}", "", "pass"),
        ("UpdateConfigurationItem", "(sa{sv})(sa{sv})", "", "pass"),
        ("GetSecretInformation", "a{sv}", "a(sa{sv})", "pass"),
        ("Format", "sa{sv}", "", "pass"),
        ("OpenForBackup", "sa{sv}", "h", "pass"),
        ("OpenForRestore", "sa{sv}", "h", "pass"),
        ("OpenForBenchmark", "sa{sv}", "h", "pass"),
        ("OpenDevice", "sa{sv}", "h", "pass"),
        ("Rescan", "a{sv}", "", "pass"),
        ("RestoreEncryptedHeader", "sa{sv}", "", "pass"),
    ]

    filesystem_props = {
        "MountPoints": dbus.Array([], signature="ay"),
        "Size": dbus.UInt64(device_size),
    }

    filesystem_methods = [
        ("SetLabel", "sa{sv}", "", "pass"),
        ("SetUUID", "sa{sv}", "", "pass"),
        ("Mount", "a{sv}", "s", "ret = self.mount_filesystem(self.block_name)"),
        ("Unmount", "a{sv}", "", "self.unmount_filesystem(self.block_name)"),
        ("Resize", "ta{sv}", "", "pass"),
        ("Check", "a{sv}", "b", "ret = self.check_filesystem(self.block_name)"),
        ("Repair", "a{sv}", "b", "ret = self.repair_filesystem(self.block_name)"),
        ("TakeOwnership", "a{sv}", "", "pass"),
    ]

    block_device_path = UDISKS2_BLOCK_DEVICES + block_full_name
    drive_device_path = UDISKS2_DRIVE_DEVICES + drive_full_name

    self.save_block_path(block_device_path)
    self.save_drive_path(drive_device_path)

    self.AddObject(drive_device_path, UDISKS2_DRIVE_IFACE, drive_props, drive_methods)

    self.AddObject(block_device_path, UDISKS2_BLOCK_IFACE, block_props, block_methods)

    drv_obj = dbusmock.get_object(drive_device_path)
    drv_obj.block_name = block_full_name
    drv_obj.drive_name = drive_full_name

    blk_obj = dbusmock.get_object(block_device_path)
    blk_obj.block_name = block_full_name
    blk_obj.drive_name = drive_full_name
    blk_obj.is_damaged = is_damaged
    blk_obj.AddProperties(UDISKS2_FILESYSTEM_IFACE, filesystem_props)
    blk_obj.AddMethods(UDISKS2_FILESYSTEM_IFACE, filesystem_methods)

    main_obj = dbusmock.get_object(MAIN_OBJ)
    main_obj.object_manager_emit_added(drive_device_path)
    main_obj.object_manager_emit_added(block_device_path)

@dbus.service.method(MOCK_IFACE,
                     in_signature="s", out_signature="")
def remove_mock_device(self, device_name):
    """
    Convenience method to remove a mock device
    """

    block_path = UDISKS2_BLOCK_DEVICES + device_name

    blk_obj = dbusmock.get_object(block_path)

    drive_path = UDISKS2_DRIVE_DEVICES + blk_obj.drive_name

    self.object_manager_emit_removed(block_path)

    self.RemoveObject(block_path)

    self.remove_block_path(block_path)

    self.remove_drive_path(drive_path)

    self.object_manager_emit_removed(drive_path)

    self.RemoveObject(drive_path)

@dbus.service.method(MOCK_IFACE,
                     in_signature="s", out_signature="s")
def mount_filesystem(self, device_name):
    """
    Convenience method to mount a mock block device object
    simulates the org.freedesktop.UDisks2.Filesystem.Mount
    """

    path = UDISKS2_BLOCK_DEVICES + device_name

    job_path = self.create_job("filesystem-mount", path)

    self.finish_job(job_path)

    mount_point = MOUNT_POINT + device_name

    obj = dbusmock.get_object(path)

    mount_points = dbus.Array([], signature="ay")
    mount_points.append(dbus.ByteArray(mount_point.encode("ascii")))

    obj.Set(UDISKS2_FILESYSTEM_IFACE, "MountPoints", mount_points)

    userspace_mount_options = dbus.Array([], signature="s")
    userspace_mount_options.append(dbus.String("uhelper=udisks2"))

    obj.Set(UDISKS2_BLOCK_IFACE, "UserspaceMountOptions", userspace_mount_options)

    return path

@dbus.service.method(MOCK_IFACE,
                     in_signature="s", out_signature="")
def unmount_filesystem(self, device_name):
    """
    Convenience method to unmount a mock block device object
    simulate org.freedesktop.UDisks2.Filesystem.Unmount
    """

    path = UDISKS2_BLOCK_DEVICES + device_name

    job_path = self.create_job("filesystem-unmount", path)

    self.finish_job(job_path)

    obj = dbusmock.get_object(path)

    mount_points = dbus.Array([], signature="ay")

    obj.Set(UDISKS2_FILESYSTEM_IFACE, "MountPoints", mount_points)

    userspace_mount_options = dbus.Array([], signature="s")

    obj.Set(UDISKS2_BLOCK_IFACE, "UserspaceMountOptions", userspace_mount_options)

@dbus.service.method(MOCK_IFACE,
                     in_signature="s", out_signature="b")
def check_filesystem(self, device_name):
    """
    Convenience method to check a mock block device object
    simulates org.freedesktop.UDisks2.Filesystem.Check
    """

    path = UDISKS2_BLOCK_DEVICES + device_name

    job_path = self.create_job("filesystem-check", path)

    self.finish_job(job_path)

    obj = dbusmock.get_object(path)

    return not obj.is_damaged

@dbus.service.method(MOCK_IFACE,
                     in_signature="s", out_signature="b")
def repair_filesystem(self, device_name):
    """
    Convenience method to repair a mock block device object
    simulates org.freedesktop.UDisks2.Filesystem.Repair
    """

    path = UDISKS2_BLOCK_DEVICES + device_name

    job_path = self.create_job("filesystem-repair", path)

    obj = dbusmock.get_object(path)

    self.finish_job(job_path)

    obj.is_damaged = False

    return True

@dbus.service.method(MOCK_IFACE,
                     in_signature="s", out_signature="")
def eject_drive(self, drive_name):
    """
    Convenience method to eject a mock block device object which
    simulates org.freedesktop.UDisks2.Drive.Eject
    """

    drive_path = UDISKS2_DRIVE_DEVICES + drive_name

    drive_obj = dbusmock.get_object(drive_path)

    block_path = UDISKS2_BLOCK_DEVICES + drive_obj.block_name

    old_block_obj = dbusmock.get_object(block_path)

    # Workaround: python-dbusmock does not support removing of individual dbus ifaces.
    # So remove the block_devices object and then add a new object without the
    # 'org.freedesktop.UDisks2.Filesystem' iface.
    block_props = {
        "Device": old_block_obj.Get(UDISKS2_BLOCK_IFACE, "Device"),
        "PreferredDevice": old_block_obj.Get(UDISKS2_BLOCK_IFACE, "PreferredDevice"),
        "Symlinks": old_block_obj.Get(UDISKS2_BLOCK_IFACE, "Symlinks"),
        "DeviceNumber": old_block_obj.Get(UDISKS2_BLOCK_IFACE, "DeviceNumber"),
        "Id": old_block_obj.Get(UDISKS2_BLOCK_IFACE, "Id"),
        "Size": old_block_obj.Get(UDISKS2_BLOCK_IFACE, "Size"),
        "ReadOnly": old_block_obj.Get(UDISKS2_BLOCK_IFACE, "ReadOnly"),
        "Drive": old_block_obj.Get(UDISKS2_BLOCK_IFACE, "Drive"),
        "MDRaid": old_block_obj.Get(UDISKS2_BLOCK_IFACE, "MDRaid"),
        "MDRaidMember": old_block_obj.Get(UDISKS2_BLOCK_IFACE, "MDRaidMember"),
        "IdUsage": old_block_obj.Get(UDISKS2_BLOCK_IFACE, "IdUsage"),
        "IdType": old_block_obj.Get(UDISKS2_BLOCK_IFACE, "IdType"),
        "IdVersion": old_block_obj.Get(UDISKS2_BLOCK_IFACE, "IdVersion"),
        "IdLabel": old_block_obj.Get(UDISKS2_BLOCK_IFACE, "IdLabel"),
        "IdUUID": old_block_obj.Get(UDISKS2_BLOCK_IFACE, "IdUUID"),
        "Configuration": old_block_obj.Get(UDISKS2_BLOCK_IFACE, "Configuration"),
        "CryptoBackingDevice": old_block_obj.Get(UDISKS2_BLOCK_IFACE, "CryptoBackingDevice"),
        "HintPartitionable": old_block_obj.Get(UDISKS2_BLOCK_IFACE, "HintPartitionable"),
        "HintSystem": old_block_obj.Get(UDISKS2_BLOCK_IFACE, "HintSystem"),
        "HintIgnore": old_block_obj.Get(UDISKS2_BLOCK_IFACE, "HintIgnore"),
        "HintAuto": old_block_obj.Get(UDISKS2_BLOCK_IFACE, "HintAuto"),
        "HintName": old_block_obj.Get(UDISKS2_BLOCK_IFACE, "HintName"),
        "HintIconName": old_block_obj.Get(UDISKS2_BLOCK_IFACE, "HintIconName"),
        "HintSymbolicIconName": old_block_obj.Get(UDISKS2_BLOCK_IFACE, "HintSymbolicIconName"),
        "UserspaceMountOptions": old_block_obj.Get(UDISKS2_BLOCK_IFACE, "UserspaceMountOptions"),
    }

    block_methods = [
        ("AddConfigurationItem", "(sa{sv})a{sv}", "", "pass"),
        ("RemoveConfigurationItem", "(sa{sv})a{sv}", "", "pass"),
        ("UpdateConfigurationItem", "(sa{sv})(sa{sv})", "", "pass"),
        ("GetSecretInformation", "a{sv}", "a(sa{sv})", "pass"),
        ("Format", "sa{sv}", "", "pass"),
        ("OpenForBackup", "sa{sv}", "h", "pass"),
        ("OpenForRestore", "sa{sv}", "h", "pass"),
        ("OpenForBenchmark", "sa{sv}", "h", "pass"),
        ("OpenDevice", "sa{sv}", "h", "pass"),
        ("Rescan", "a{sv}", "", "pass"),
        ("RestoreEncryptedHeader", "sa{sv}", "", "pass"),
    ]

    changed_block_props = {
        "HintAuto": dbus.Boolean(0),
        "IdUUID": dbus.String(""),
        "IdVersion": dbus.String(""),
        "IdType": dbus.String(""),
        "IdUsage": dbus.String(""),
        "Id": dbus.String(""),
        "Size": dbus.UInt64(0),
    }

    changed_drive_props = {
        "TimeMediaDetected": dbus.UInt64(0),
        "MediaAvailable": dbus.Boolean(False),
        "Size": dbus.UInt64(0),
    }

    job_path = self.create_job("drive-eject", drive_path)

    self.finish_job(job_path)

    self.RemoveObject(block_path)

    self.AddObject(block_path, UDISKS2_BLOCK_IFACE, block_props, block_methods)

    main_obj = dbusmock.get_object(MAIN_OBJ)

    drive_obj.UpdateProperties(UDISKS2_DRIVE_IFACE, changed_drive_props)

    new_block_obj = dbusmock.get_object(block_path)
    new_block_obj.block_name = old_block_obj.block_name
    new_block_obj.drive_name = old_block_obj.drive_name
    new_block_obj.is_damaged = old_block_obj.is_damaged
    new_block_obj.UpdateProperties(UDISKS2_BLOCK_IFACE, changed_block_props)

    main_obj.EmitSignal(
        dbusmock.OBJECT_MANAGER_IFACE, "InterfacesRemoved", "oas", [dbus.ObjectPath(block_path), [dbus.String(UDISKS2_FILESYSTEM_IFACE)]]
    )

@dbus.service.method(MOCK_IFACE,
                     in_signature="ss", out_signature="s")
def create_job(self, operation, device_path) -> str:
    """
    Convenience method to create a job
    simulates org.freedesktop.UDisks2.Job
    """

    mng_obj = dbusmock.get_object(UDISKS2_MANAGER_OBJ)

    job_path = UDISKS2_JOBS + str(mng_obj.job_counter)
    mng_obj.job_counter += 1

    dt = datetime.now()

    props = {
        "Operation": dbus.String(operation),
        "Progress": dbus.Double(0),
        "ProgressValid": dbus.Boolean(False),
        "Bytes": dbus.UInt64(0),
        "Rate": dbus.UInt64(0),
        "StartTime": dbus.UInt64(dt.timestamp()),
        "ExpectedEndTime": dbus.UInt64(0),
        "Objects": dbus.Array([device_path], signature="o"),
        "StartedByUID": dbus.UInt32(0),
        "Cancelable": dbus.Boolean(False),
    }

    methods = [
        ("Cancel", "a{sv}", "", "pass"),
    ]

    self.AddObject(job_path, UDISKS2_JOB_IFACE, props, methods)

    self.object_manager_emit_added(job_path)

    return job_path

@dbus.service.method(MOCK_IFACE,
                     in_signature="s", out_signature="")
def finish_job(self, job_path):
    """
    Convenience method to remove a job
    simulates org.freedesktop.UDisks2.Job
    """

    obj = dbusmock.get_object(job_path)

    obj.EmitSignal(UDISKS2_JOB_IFACE, "Completed", "bs", [True,""])

    self.object_manager_emit_removed(job_path)

    self.RemoveObject(job_path)

@dbus.service.method(MOCK_IFACE,
                     in_signature="s", out_signature="b")
def can_format(_self, filesystem):
    #TODO: implement
    return False, 0

@dbus.service.method(MOCK_IFACE,
                     in_signature="s", out_signature="(bt)")
def can_resize(_self, filesystem):
    #TODO: implement
    return False, 0

@dbus.service.method(MOCK_IFACE,
                     in_signature="s", out_signature="b")
def can_check(_self, filesystem):
    """
    Convenience method to check if filesystem can be checked
    simulates org.freedesktop.UDisks2.Manager.CanCheck
    """

    obj = dbusmock.get_object(UDISKS2_MANAGER_OBJ)
    return any(elem in filesystem for elem in obj.supported_filesystems)

@dbus.service.method(MOCK_IFACE,
                     in_signature="s", out_signature="b")
def can_repair(_self, filesystem):
    """
    Convenience method to check if filesystem can be repaired
    simulates org.freedesktop.UDisks2.Manager.CanRepair
    """

    obj = dbusmock.get_object(UDISKS2_MANAGER_OBJ)
    return any(elem in filesystem for elem in obj.supported_filesystems)

@dbus.service.method(MOCK_IFACE,
                     in_signature="s", out_signature="")
def save_block_path(_self, path):
    """
    Convenience method to maintain a list of block devices which is used by
    org.freedesktop.UDisks2.Manager.GetBlockDevices
    """

    obj = dbusmock.get_object(UDISKS2_MANAGER_OBJ)
    obj.block_devices.append(str(path))

@dbus.service.method(MOCK_IFACE,
                     in_signature="s", out_signature="")
def remove_block_path(_self, path):
    """
    Convenience method to maintain a list of block devices which is used by
    org.freedesktop.UDisks2.Manager.GetBlockDevices
    """

    obj = dbusmock.get_object(UDISKS2_MANAGER_OBJ)
    obj.block_devices.remove(str(path))

@dbus.service.method(MOCK_IFACE,
                     in_signature="", out_signature="ao")
def get_block_paths(_self) -> list[str]:
    """
    Convenience method to maintain a list of block devices which is used by
    org.freedesktop.UDisks2.Manager.GetBlockDevices
    """

    obj = dbusmock.get_object(UDISKS2_MANAGER_OBJ)
    return obj.block_devices

@dbus.service.method(MOCK_IFACE,
                     in_signature="s", out_signature="")
def save_drive_path(_self, path):
    """
    Convenience method to maintain a list of drive devices which is used by
    org.freedesktop.UDisks2.Manager.GetDrives
    """

    obj = dbusmock.get_object(UDISKS2_MANAGER_OBJ)
    obj.drive_devices.append(str(path))

@dbus.service.method(MOCK_IFACE,
                     in_signature="s", out_signature="")
def remove_drive_path(_self, path):
    """
    Convenience method to maintain a list of drive devices which is used by
    org.freedesktop.UDisks2.Manager.GetDrives
    """

    obj = dbusmock.get_object(UDISKS2_MANAGER_OBJ)
    obj.drive_devices.remove(str(path))

@dbus.service.method(MOCK_IFACE,
                     in_signature="", out_signature="ao")
def get_drive_paths(_self) -> list[str]:
    """
    Convenience method to maintain a list of drive devices which is used by
    org.freedesktop.UDisks2.Manager.GetDrives
    """

    obj = dbusmock.get_object(UDISKS2_MANAGER_OBJ)
    return obj.drive_devices
