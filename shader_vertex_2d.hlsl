/*==============================================================================

   2D�`��p���_�V�F�[�_�[ [shader_vertex_2d.hlsl]
--------------------------------------------------------------------------------

==============================================================================*/

// �萔�o�b�t�@
//float4x4 mtx; //C���ꂩ��n���ꂽ�f�[�^�������Ă���


// �萔�o�b�t�@�iC���ꂩ��n�����j
cbuffer CB0 : register(b0)
{
    float4x4 mtx; // �ϊ��s��i�g��������Ύg����j
    float2 screenSize; // ��ʃT�C�Y�i��: 1280, 720�j
    float2 padding; // �p�f�B���O�ifloat4x4 �ɍ��킹�邽�߁j
}

//���͗p���_�\����
struct VS_INPUT
{ //              V �R�����I
    float4 posL : POSITION0; //���_���W �I�[�łȂ��[���I
    float4 color : COLOR0; //���_�J���[�iR,G,B,A�j
    float2 texcorrd : TEXCOORD0;
};

//�o�͗p���_�\����
struct VS_OUTPUT
{
    float4 posH : SV_POSITION; //�ϊ��ϒ��_���W
    float4 color : COLOR0; //���_�J���[
    float2 texcorrd : TEXCOORD0;
};

VS_OUTPUT main(VS_INPUT vs_in)
{
    
    VS_OUTPUT vs_out; //�o�͗p�\���̕ϐ�

    // ���_���W�ɍs���K�p�i�K�v�Ȃ�j
    float4 transformed = mul(vs_in.posL, mtx);

    // ���[�J����NDC�ϊ��iSwitch �������W�n �� DirectX ���W�n�j
    float2 ndc;
    ndc.x = transformed.x / (screenSize.x * 0.5f);
    ndc.y = -transformed.y / (screenSize.y * 0.5f); // Y�����]
    
    //���_���s��ŕϊ�
    vs_out.posH = mul(vs_in.posL, mtx);
    //vs_out.posH = float4(ndc, 0.0f, 1.0f);
    //���_�J���[�͂��̂܂܏o��
    vs_out.color = vs_in.color;
    vs_out.texcorrd = vs_in.texcorrd;
    //���ʂ��o�͂���
    return vs_out;
}



////=============================================================================
//// ���_�V�F�[�_
////=============================================================================
//float4 main(in float4 posL : POSITION0 ) : SV_POSITION
//{
//	return mul(posL, mtx);//���_���W��mtx�i�ϊ��s��j
//}
