#include <catch2/catch_test_macros.hpp>

#include <cstdint>
#include <string>
#include <vector>

#include <rex/codegen/codegen_context.h>
#include <rex/codegen/test_support.h>

#include "codegen/builders.h"
#include "codegen/builders/builder_context.h"
#include "codegen/ppc/disasm.h"

using namespace rex::codegen;

TEST_CASE("PPC sync builders emit host memory fences", "[codegen][ppc][sync]") {
  constexpr uint32_t kBaseAddress = 0x82000000;
  std::vector<uint8_t> code{0x4E, 0x80, 0x00, 0x20};  // blr
  TestModule module;
  module.Load(kBaseAddress, code.data(), code.size());

  auto context = CodegenContext::Create(BinaryView::fromModule(module), RecompilerConfig{});
  auto* function = context.graph.addFunction(kBaseAddress, 4, FunctionAuthority::DISCOVERED, true);
  REQUIRE(function != nullptr);
  function->discover({{kBaseAddress, 4}}, {}, {});
  function->seal();

  EmitContext emitContext{context.binary(), context.Config(), context.graph};
  ppc_insn instruction{};
  uint32_t rawInstruction = 0;
  RecompilerLocalVariables locals;
  CSRState csrState = CSRState::Unknown;
  std::string output;
  BuilderContext builder{output, emitContext, *function, instruction, kBaseAddress,
                         &rawInstruction, locals, csrState};

  CHECK(build_lwsync(builder));
  CHECK(output == "\tstd::atomic_thread_fence(std::memory_order_acq_rel);\n");

  output.clear();
  CHECK(build_sync(builder));
  CHECK(output == "\tstd::atomic_thread_fence(std::memory_order_seq_cst);\n");
}
