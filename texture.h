#pragma once
#include <string>


unsigned int LoadTexture(std::string FileName);
void UnloadTexture(unsigned int Texture);
ID3D11ShaderResourceView* SetTexture(unsigned int Texture);
std::vector<ID3D11ShaderResourceView*>* GetTexture();