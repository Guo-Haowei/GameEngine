#pragma once

void R_CreateQuad();
void R_DrawQuad();

namespace gl {

uint64_t MakeTextureResident(uint32_t texture);

}  // namespace gl
