#include "bq27220.h"
#include "bq27220_data_memory.h"

#define BQ27220_ID (0x0220u)

/** Delay between 2 writes into Subclass/MAC area. Fails at ~120us. */
#define BQ27220_MAC_WRITE_DELAY_US (250u)

/** Delay between we ask chip to load data to MAC and it become valid. Fails at ~500us. */
#define BQ27220_SELECT_DELAY_US (1000u)

/** Delay between 2 control operations(like unseal or full access). Fails at ~2500us.*/
#define BQ27220_MAGIC_DELAY_US (5000u)

/** Delay before freshly written configuration can be read. Fails at ? */
#define BQ27220_CONFIG_DELAY_US (10000u)

/** Config apply delay. Must wait, or DM read returns garbage. */
#define BQ27220_CONFIG_APPLY_US (2000000u)

/** Timeout for common operations. */
#define BQ27220_TIMEOUT_COMMON_US (2000000u)

/** Timeout for reset operation. Normally reset takes ~2s. */
#define BQ27220_TIMEOUT_RESET_US (4000000u)

/** Timeout cycle interval  */
#define BQ27220_TIMEOUT_CYCLE_INTERVAL_US (1000u)

/** Timeout cycles count helper */
#define BQ27220_TIMEOUT(timeout_us) ((timeout_us) / (BQ27220_TIMEOUT_CYCLE_INTERVAL_US))

static uint8_t bq27220_get_checksum(uint8_t* data, uint16_t len) {
    uint8_t ret = 0;
    for(uint16_t i = 0; i < len; i++) {
        ret += data[i];
    }
    return 0xFF - ret;
}

bool BQ27220::parameterCheck(uint16_t address, uint32_t value, size_t size, bool update)
{
    if(!(size == 1 || size == 2 || size == 4)) {
        return false;
    }

    bool ret = false;
    uint8_t buffer[6] = {0};
    uint8_t old_data[4] = {0};

    do {
        buffer[0] = address & 0xFF;
        buffer[1] = (address >> 8) & 0xFF;

        for(size_t i = 0; i < size; i++) {
            buffer[1 + size - i] = (value >> (i * 8)) & 0xFF;
        }

        if(update) {
            if(!i2cWriteBytes(CommandSelectSubclass, buffer, size + 2)) {
                break;
            }
            delayMicroseconds(BQ27220_MAC_WRITE_DELAY_US);

            uint8_t checksum = bq27220_get_checksum(buffer, size + 2);
            buffer[0] = checksum;
            buffer[1] = 2 + size + 1 + 1;
            if(!i2cWriteBytes(CommandMACDataSum, buffer, size + 2)) {
                break;
            }
            delayMicroseconds(BQ27220_CONFIG_DELAY_US);
            ret = true;
        } else {
            if(!i2cWriteBytes(CommandSelectSubclass, buffer, 2)) {
                break;
            }
            delayMicroseconds(BQ27220_SELECT_DELAY_US);

            if(!i2cReadBytes(CommandMACData, old_data, size)) {
                break;
            }
            delayMicroseconds(BQ27220_SELECT_DELAY_US);

            if(*(uint32_t*)&(old_data[0]) != *(uint32_t*)&(buffer[2])) {
            } else {
                ret = true;
            }
        }
    } while(0);

    return ret;
}

bool BQ27220::dateMemoryCheck(const BQ27220DMData *data_memory, bool update)
{
    if(update) {
        const uint16_t cfg_request = Control_ENTER_CFG_UPDATE;
        if(!i2cWriteBytes(CommandSelectSubclass, (uint8_t*)&cfg_request, sizeof(cfg_request))) {
            return false;
        }

        uint32_t timeout = BQ27220_TIMEOUT(BQ27220_TIMEOUT_COMMON_US);
        BQ27220OperationStatus operation_status;
        while(--timeout > 0) {
            if(!getOperationStatus(&operation_status)) {
            } else if(operation_status.reg.CFGUPDATE) {
                break;
            };
            delayMicroseconds(BQ27220_TIMEOUT_CYCLE_INTERVAL_US);
        }

        if(timeout == 0) {
            return false;
        }
    }

    // Process data memory records
    bool result = true;
    while (data_memory->type != BQ27220DMTypeEnd)
    {
        if(data_memory->type == BQ27220DMTypeWait) {
            delayMicroseconds(data_memory->value.u32);
        } else if(data_memory->type == BQ27220DMTypeU8) {
            result &= parameterCheck(data_memory->address, data_memory->value.u8, 1, update);
        } else if(data_memory->type == BQ27220DMTypeU16) {
            result &= parameterCheck(data_memory->address, data_memory->value.u16, 2, update);
        } else if(data_memory->type == BQ27220DMTypeU32) {
            result &= parameterCheck(data_memory->address, data_memory->value.u32, 4, update);
        } else if(data_memory->type == BQ27220DMTypeI8) {
            result &= parameterCheck(data_memory->address, data_memory->value.i8, 1, update);
        } else if(data_memory->type == BQ27220DMTypeI16) {
            result &= parameterCheck(data_memory->address, data_memory->value.i16, 2, update);
        } else if(data_memory->type == BQ27220DMTypeI32) {
            result &= parameterCheck(data_memory->address, data_memory->value.i32, 4, update);
        } else if(data_memory->type == BQ27220DMTypeF32) {
            result &= parameterCheck(data_memory->address, data_memory->value.u32, 4, update);
        } else if(data_memory->type == BQ27220DMTypePtr8) {
            result &= parameterCheck(data_memory->address, *(uint8_t*)data_memory->value.u32, 1, update);
        } else if(data_memory->type == BQ27220DMTypePtr16) {
            result &= parameterCheck(data_memory->address, *(uint16_t*)data_memory->value.u32, 2, update);
        } else if(data_memory->type == BQ27220DMTypePtr32) {
            result &= parameterCheck(data_memory->address, *(uint32_t*)data_memory->value.u32, 4, update);
        } else {
        }
        data_memory++;
    }
    
    // Finalize configuration update
    if(update && result) {
        controlSubCmd(Control_EXIT_CFG_UPDATE_REINIT);

        // Wait for gauge to apply new configuration
        delayMicroseconds(BQ27220_CONFIG_APPLY_US);

        // ensure that we exited config update mode
        uint32_t timeout = BQ27220_TIMEOUT(BQ27220_TIMEOUT_COMMON_US);
        BQ27220OperationStatus operation_status;
        while(--timeout > 0) {
            if(!getOperationStatus(&operation_status)) {
            } else if(operation_status.reg.CFGUPDATE != true) {
                break;
            }
            delayMicroseconds(BQ27220_TIMEOUT_CYCLE_INTERVAL_US);
        }

        // Check timeout
        if(timeout == 0) {
            return false;
        }
    }
    return result;
}

bool BQ27220::init(const BQ27220DMData *data_memory)
{
    bool result = false;
    bool reset_and_provisioning_required = false;

    do{
        uint16_t data = getDeviceNumber();
        if(data != BQ27220_ID) {
            break;
        }
        
        // Unseal device since we are going to read protected configuration
        if(!unsealAccess()) {
            break;
        }

        // Try to recover gauge from forever init
        BQ27220OperationStatus operat;
        if(!getOperationStatus(&operat)) {
            break;
        }
        if(!operat.reg.INITCOMP || operat.reg.CFGUPDATE) {
            reset_and_provisioning_required = true;
        }

        // Ensure correct profile is selected
        BQ27220ControlStatus control_status;
        if(!getControlStatus(&control_status)) {
            break;
        }
        if(control_status.reg.BATT_ID != 0) {
            reset_and_provisioning_required = true;
        }

        // Ensure correct configuration loaded into gauge DataMemory
        // Only if reset is not required, otherwise we don't
        if(!reset_and_provisioning_required) {
            if(!dateMemoryCheck(data_memory, false)) {
                reset_and_provisioning_required = true;
            }
        }

        // Reset needed
        if(reset_and_provisioning_required) {
            if(!reset()) {
            }

            // Get full access to read and modify parameters
            // Also it looks like this step is totally unnecessary
            if(!fullAccess()) {
                break;
            }

            // Update memory
            dateMemoryCheck(data_memory, true);
            if(!dateMemoryCheck(data_memory, false)) {
                break;
            }
        }

        if(!sealAccess()) {
            break;
        }

    } while(0);
    return result;
}

bool BQ27220::reset(void)
{
    bool result = false;
    do{
        controlSubCmd(Control_RESET);
        // delay(10);

        uint32_t timeout = BQ27220_TIMEOUT(BQ27220_TIMEOUT_RESET_US);
        BQ27220OperationStatus operat = {0};
        while (--timeout > 0)
        {
            if(!getOperationStatus(&operat)){
            }else if(operat.reg.INITCOMP == true){
                break;
            }
            delayMicroseconds(BQ27220_TIMEOUT_CYCLE_INTERVAL_US); // delay(2);
        }
        if(timeout == 0) {
            break;
        }
        result = true;
    } while(0);
    return result;
}

// Sealed Access
bool BQ27220::sealAccess(void) 
{
    bool result = false;
    BQ27220OperationStatus operat = {0};
    do{
        getOperationStatus(&operat);
        if(operat.reg.SEC == Bq27220OperationStatusSecSealed)
        {
            result = true;
            break;
        }

        controlSubCmd(Control_SEALED);
        // delay(10);
        delayMicroseconds(BQ27220_SELECT_DELAY_US);

        getOperationStatus(&operat);
        if(operat.reg.SEC != Bq27220OperationStatusSecSealed)
        {
            break;
        }
        result = true;
    } while(0);

    return result;
}

bool BQ27220::unsealAccess(void) 
{
    bool result = false;
    BQ27220OperationStatus operat = {0};
    do{
        getOperationStatus(&operat);
        if(operat.reg.SEC != Bq27220OperationStatusSecSealed)
        {
            result = true;
            break;
        }

        controlSubCmd(UnsealKey1);
        delayMicroseconds(BQ27220_MAGIC_DELAY_US); // delay(10);
        controlSubCmd(UnsealKey2);
        delayMicroseconds(BQ27220_MAGIC_DELAY_US);  // delay(10);

        getOperationStatus(&operat);
        if(operat.reg.SEC != Bq27220OperationStatusSecUnsealed)
        {
            break;
        }
        result = true;
    } while (0);

    return result;
}

bool BQ27220::fullAccess(void) 
{
    bool result = false;
    BQ27220OperationStatus operat = {0};

    do{
        uint32_t timeout = BQ27220_TIMEOUT(BQ27220_TIMEOUT_COMMON_US);
        while (--timeout > 0)
        {
            if(!getOperationStatus(&operat)){
            }else {
                break;
            }
        }
        if(timeout == 0) {
            break;
        }

        // Already full access
        if(operat.reg.SEC == Bq27220OperationStatusSecFull){
            result = true;
            break;
        }
        // Must be unsealed to get full access
        if(operat.reg.SEC != Bq27220OperationStatusSecUnsealed){
            break;
        }

        controlSubCmd(FullAccessKey);
        delayMicroseconds(BQ27220_MAGIC_DELAY_US); //delay(10);
        controlSubCmd(FullAccessKey);
        delayMicroseconds(BQ27220_MAGIC_DELAY_US); //delay(10);

        if(!getOperationStatus(&operat)){
            break;
        }
        if(operat.reg.SEC != Bq27220OperationStatusSecFull){
            break;
        }
        result = true;
    } while (0);
    return result;
}

uint16_t BQ27220::getDeviceNumber(void)
{
    uint16_t devid = 0;
    // Request device number(chip PN)
    controlSubCmd(Control_DEVICE_NUMBER);
    // Enterprise wait(MAC read fails if less than 500us)
    // bqstudio uses ~15ms 
    delayMicroseconds(BQ27220_SELECT_DELAY_US); // delay(15);
    // Read id data from MAC scratch space
    i2cReadBytes(CommandMACData, (uint8_t *)&devid, 2);

    return devid;
}

uint16_t BQ27220::getVoltage(void)
{
    return readRegU16(CommandVoltage);
}
int16_t BQ27220::getCurrent(void)
{
    return readRegU16(CommandCurrent);
}
bool BQ27220::getControlStatus(BQ27220ControlStatus *ctrl_sta)
{
    (*ctrl_sta).full = readRegU16(CommandControl);
    return true;
}
bool BQ27220::getBatteryStatus(BQ27220BatteryStatus *batt_sta)
{
    (*batt_sta).full = readRegU16(CommandBatteryStatus);
    return true;
}
bool BQ27220::getOperationStatus(BQ27220OperationStatus *oper_sta)
{
    (*oper_sta).full = readRegU16(CommandOperationStatus);
    return true;
}
bool BQ27220::getGaugingStatus(BQ27220GaugingStatus *gauging_sta)
{
    // Request gauging data to be loaded to MAC
    controlSubCmd(Control_GAUGING_STATUS);
    // Wait for data being loaded to MAC
    delayMicroseconds(BQ27220_SELECT_DELAY_US);
    // Read id data from MAC scratch space
    (*gauging_sta).full = readRegU16(CommandMACData);
    return true;
}
uint16_t BQ27220::getTemperature(void)
{
    return readRegU16(CommandTemperature);
}
uint16_t BQ27220::getFullChargeCapacity(void)
{
    return readRegU16(CommandFullChargeCapacity);
}
uint16_t BQ27220::getDesignCapacity(void)
{
    return readRegU16(CommandDesignCapacity);
}
uint16_t BQ27220::getRemainingCapacity(void)
{
    return readRegU16(CommandRemainingCapacity);
}
uint16_t BQ27220::getStateOfCharge(void)
{
    return readRegU16(CommandStateOfCharge);
}
uint16_t BQ27220::getStateOfHealth(void)
{
    return readRegU16(CommandStateOfHealth);
}
uint16_t BQ27220::getChargeVoltageMax(void)
{
    return readRegU16(CommandChargeVoltage);
}