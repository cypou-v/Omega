#include "external_flash.h"

namespace Ion {
namespace ExternalFlash {
namespace Device {

/* The external flash and the Quad-SPI peripheral support
 * several operating modes, corresponding to different numbers of signals
 * used to communicate during each phase of the command sequence.
 *
 *   Mode name for | Number of signals used during each phase:
 *  external flash | Instruction | Address | Alt. bytes | Data
 * ----------------+-------------+---------+------------+------
 * Standard    SPI |      1      |    1    |     1      |   1
 * Dual-Output SPI |      1      |    1    |     1      |   2
 * Dual-I/O    SPI |      1      |    2    |     2      |   2
 * Quad-Output SPI |      1      |    1    |     1      |   4
 * Quad-I/O    SPI |      1      |    4    |     4      |   4
 *             QPI |      4      |    4    |     4      |   4
 *
 * The external flash supports clock frequencies up to 104MHz for all instructions,
 * except for Read Data (0x03) which is supported up to 50Mhz.
 *
 * After the external flash receives a Read instructions and shifts a byte out,
 * it automatically increments the provided address and shifts out the corresponding byte,
 * and so on as long as the clock continues, allowing for a continuous stream of data.
 */

static constexpr QUADSPI::CCR::OperatingMode DefaultOperatingMode = QUADSPI::CCR::OperatingMode::Single;

static void send_command_full(QUADSPI::CCR::FunctionalMode functionalMode, QUADSPI::CCR::OperatingMode operatingMode, Command c, uint32_t address, uint8_t dummyCycles, uint8_t * data, size_t dataLength);

static inline void send_command(Command c) {
  send_command_full(
    QUADSPI::CCR::FunctionalMode::IndirectWrite,
    DefaultOperatingMode,
    c,
    0, 0, nullptr, 0
  );
}

static inline void send_command_single(Command c) {
  send_command_full(
    QUADSPI::CCR::FunctionalMode::IndirectWrite,
    QUADSPI::CCR::OperatingMode::Single,
    c,
    0, 0, nullptr, 0
  );
}

static inline void send_write_command(Command c, uint32_t address, uint8_t * data, size_t dataLength) {
  send_command_full(
    QUADSPI::CCR::FunctionalMode::IndirectWrite,
    DefaultOperatingMode,
    c,
    address, 0, data, dataLength
  );
}

static inline void send_read_command(Command c, uint32_t address, uint8_t * data, size_t dataLength) {
  send_command_full(
    QUADSPI::CCR::FunctionalMode::IndirectRead,
    DefaultOperatingMode,
    c,
    address, 0, data, dataLength
  );
}

static inline void wait() {
  class ExternalFlashStatusRegister : Register16 {
  public:
    using Register16::Register16;
    REGS_BOOL_FIELD(BUSY, 0);
  };
  ExternalFlashStatusRegister statusRegister(0);
  do {
    send_read_command(Command::ReadStatusRegister, 0, reinterpret_cast<uint8_t *>(&statusRegister), sizeof(statusRegister));
  } while (statusRegister.getBUSY());
}

static void set_as_memory_mapped() {
  send_command_full(
    QUADSPI::CCR::FunctionalMode::MemoryMapped,
    DefaultOperatingMode,
    Command::ReadData,
    0, 0, nullptr, 0
  );
}

void send_command_full(QUADSPI::CCR::FunctionalMode functionalMode, QUADSPI::CCR::OperatingMode operatingMode, Command c, uint32_t address, uint8_t dummyCycles, uint8_t * data, size_t dataLength) {
  class QUADSPI::CCR ccr(0);
  ccr.setFMODE(functionalMode);
  if (data != nullptr || functionalMode == QUADSPI::CCR::FunctionalMode::MemoryMapped) {
    ccr.setDMODE(operatingMode);
  }
  QUADSPI.DLR()->set((dataLength > 0) ? dataLength-1 : 0);
  ccr.setDCYC(dummyCycles);
  if (address != 0 || functionalMode == QUADSPI::CCR::FunctionalMode::MemoryMapped) {
    ccr.setADMODE(operatingMode);
    ccr.setADSIZE(QUADSPI::CCR::Size::ThreeBytes);
  }
  ccr.setIMODE(operatingMode);
  ccr.setINSTRUCTION(static_cast<uint8_t>(c));
  QUADSPI.CCR()->set(ccr);
  if (address != 0) {
    QUADSPI.AR()->set(address);
  }

  // FIXME: Handle access sizes
  if (functionalMode == QUADSPI::CCR::FunctionalMode::IndirectWrite) {
    for (size_t i=0; i<dataLength; i++) {
      QUADSPI.DR()->set(data[i]);
    }
  } else if (functionalMode == QUADSPI::CCR::FunctionalMode::IndirectRead) {
    for (size_t i=0; i<dataLength; i++) {
      data[i] = QUADSPI.DR()->get();
    }
  }

  /* Wait for the command to be sent.
   * "When configured in memory-mapped mode, because of the prefetch operations,
   * BUSY does not fall until there is a timeout, there is an abort, or the
   * peripheral is disabled." */
  if (functionalMode != QUADSPI::CCR::FunctionalMode::MemoryMapped) {
    while (QUADSPI.SR()->getBUSY()) {
    }
  }
}

void init() {
  initGPIO();
  initQSPI();
  initChip();
}

void initGPIO() {
  for(const GPIOPin & g : QSPIPins) {
    g.group().MODER()->setMode(g.pin(), GPIO::MODER::Mode::AlternateFunction);
    g.group().AFR()->setAlternateFunction(g.pin(), (g.pin() == 6 ? GPIO::AFR::AlternateFunction::AF10 : GPIO::AFR::AlternateFunction::AF9));
  }
}

void initQSPI() {
  // Enable QUADSPI AHB3 peripheral clocks
  RCC.AHB3ENR()->setQSPIEN(true);
  // Configure controller for target device
  QUADSPI.DCR()->setFSIZE(FlashNumberOfAddressBits-1);

  QUADSPI.DCR()->setCSHT(7); // Highest value. TODO: make it optimal
  QUADSPI.CR()->setPRESCALER(255); // Highest value. TODO: make it optimal

  QUADSPI.CR()->setEN(true);
}

void initChip() {
  /* The chip initially expects commands in SPI mode. We need to use SPI to tell
   * it to switch to QPI, hence the "_single". */
  if (DefaultOperatingMode == QUADSPI::CCR::OperatingMode::Quad) {
   send_command_single(Command::EnableQPI);
  }
  set_as_memory_mapped();
}

void MassErase() {
  send_command(Command::WriteEnable);
  send_command(Command::ChipErase);
  //send_command(ReadStatusRegister);
  wait();
  set_as_memory_mapped();
}

void EraseSector() {
  send_command(Command::WriteEnable);
  //send_command(Command::BlockErase /* PICK SIZE */, addressOfSector);
  wait();
  set_as_memory_mapped();
}

void WriteMemory(uint32_t * source, uint32_t * destination, size_t length) {
  send_command(Command::WriteEnable);
  //send_write_command(Command::QuadPageProgram, (destination-reinterpret_cast<uint32_t *>(QSPIBaseAddress)), source, length);
  // TODO: Apprently, here we must issue a new send_command every 256 byte
  wait();
  set_as_memory_mapped();
}

}
}
}
