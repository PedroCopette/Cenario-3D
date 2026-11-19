// Copyright
// Computação Gráfica
// URI Santiago
// Professor Laurence

#ifndef MODEL_HANDLER_H
#define MODEL_HANDLER_H

#include <GL/glew.h>

#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <assimp/Importer.hpp>
#include <assimp/material.h>

#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <limits>
#include <memory>
#include <string>
#include <unordered_map>

#include "stb_image.h"


// ==========================================================
// TEXTURA
// ==========================================================

struct GlbTexture {

    GLuint id = 0;

    int width = 0;

    int height = 0;

};


// ==========================================================
// MODELO
// ==========================================================

struct GlbModel {

    std::unique_ptr<Assimp::Importer> importer;

    const aiScene* scene = nullptr;

    std::unordered_map<unsigned int, GlbTexture> textures;

    aiVector3D min_bound;

    aiVector3D max_bound;

    aiVector3D center;

    float max_dimension = 1.0f;

};


// ==========================================================
// PROCURA TEXTURA
// ==========================================================

static inline bool load_texture_index_from_material(
    unsigned int& texture_index,
    const aiMaterial* material
) {

    if (!material) {
        return false;
    }

    aiString texture_path;


    // PBR / Base Color

    if (
        material->GetTextureCount(
            aiTextureType_BASE_COLOR
        ) > 0
    ) {

        if (
            material->GetTexture(
                aiTextureType_BASE_COLOR,
                0,
                &texture_path
            ) == AI_SUCCESS
        ) {

            std::string path =
                texture_path.C_Str();

            if (
                !path.empty() &&
                path[0] == '*'
            ) {

                texture_index =
                    std::strtoul(
                        path.c_str() + 1,
                        nullptr,
                        10
                    );

                return true;
            }
        }
    }


    // Diffuse

    if (
        material->GetTextureCount(
            aiTextureType_DIFFUSE
        ) > 0
    ) {

        if (
            material->GetTexture(
                aiTextureType_DIFFUSE,
                0,
                &texture_path
            ) == AI_SUCCESS
        ) {

            std::string path =
                texture_path.C_Str();

            if (
                !path.empty() &&
                path[0] == '*'
            ) {

                texture_index =
                    std::strtoul(
                        path.c_str() + 1,
                        nullptr,
                        10
                    );

                return true;
            }
        }
    }

    return false;
}


// ==========================================================
// CARREGA TEXTURA
// ==========================================================

static inline GlbTexture load_texture(
    const uint8_t* texture_data,
    GLenum internal_format,
    int texture_width,
    int texture_height,
    GLenum input_format
) {

    GLint previous_texture_bind;

    GLint previous_unpack_alignment;


    glGetIntegerv(
        GL_TEXTURE_BINDING_2D,
        &previous_texture_bind
    );

    glGetIntegerv(
        GL_UNPACK_ALIGNMENT,
        &previous_unpack_alignment
    );


    GlbTexture texture;


    glGenTextures(
        1,
        &texture.id
    );


    glBindTexture(
        GL_TEXTURE_2D,
        texture.id
    );


    texture.width =
        texture_width;

    texture.height =
        texture_height;


    glPixelStorei(
        GL_UNPACK_ALIGNMENT,
        1
    );


    glTexImage2D(
        GL_TEXTURE_2D,
        0,
        internal_format,
        texture_width,
        texture_height,
        0,
        input_format,
        GL_UNSIGNED_BYTE,
        texture_data
    );


    glTexParameteri(
        GL_TEXTURE_2D,
        GL_TEXTURE_MIN_FILTER,
        GL_LINEAR_MIPMAP_LINEAR
    );

    glTexParameteri(
        GL_TEXTURE_2D,
        GL_TEXTURE_MAG_FILTER,
        GL_LINEAR
    );

    glTexParameteri(
        GL_TEXTURE_2D,
        GL_TEXTURE_WRAP_S,
        GL_REPEAT
    );

    glTexParameteri(
        GL_TEXTURE_2D,
        GL_TEXTURE_WRAP_T,
        GL_REPEAT
    );


    glGenerateMipmap(
        GL_TEXTURE_2D
    );


    glPixelStorei(
        GL_UNPACK_ALIGNMENT,
        previous_unpack_alignment
    );


    glBindTexture(
        GL_TEXTURE_2D,
        previous_texture_bind
    );


    return texture;
}


// ==========================================================
// TEXTURA EMBUTIDA
// ==========================================================

static inline bool load_texture_embedded(
    GlbTexture& glb_texture,
    const aiTexture* assimp_texture
) {

    if (!assimp_texture) {
        return false;
    }


    const uint8_t* data =
        reinterpret_cast<const uint8_t*>(
            assimp_texture->pcData
        );


    // Textura comprimida

    if (
        assimp_texture->mHeight == 0
    ) {

        int texture_length =
            assimp_texture->mWidth;

        int texture_width;

        int texture_height;


        uint8_t* decoded =
            stbi_load_from_memory(
                data,
                texture_length,
                &texture_width,
                &texture_height,
                nullptr,
                4
            );


        if (!decoded) {
            return false;
        }


        glb_texture =
            load_texture(
                decoded,
                GL_RGBA,
                texture_width,
                texture_height,
                GL_RGBA
            );


        stbi_image_free(
            decoded
        );


        return true;
    }


    // Textura não comprimida

    glb_texture =
        load_texture(
            data,
            GL_RGBA,
            assimp_texture->mWidth,
            assimp_texture->mHeight,
            GL_BGRA
        );


    return true;
}


// ==========================================================
// LIMITES DO MODELO
// ==========================================================

static inline void calculate_model_bounds_recursive(
    const aiScene* scene,
    const aiNode* node,
    const aiMatrix4x4& parent_matrix,
    aiVector3D& min_bound,
    aiVector3D& max_bound
) {

    if (!scene || !node) {
        return;
    }


    aiMatrix4x4 current_matrix =
        parent_matrix * node->mTransformation;


    for (
        unsigned int mesh_index = 0;
        mesh_index < node->mNumMeshes;
        mesh_index++
    ) {

        const aiMesh* mesh =
            scene->mMeshes[
                node->mMeshes[mesh_index]
            ];


        if (!mesh) {
            continue;
        }


        for (
            unsigned int vertex_index = 0;
            vertex_index < mesh->mNumVertices;
            vertex_index++
        ) {

            aiVector3D vertex =
                current_matrix *
                mesh->mVertices[vertex_index];


            min_bound.x =
                std::min(
                    min_bound.x,
                    vertex.x
                );

            min_bound.y =
                std::min(
                    min_bound.y,
                    vertex.y
                );

            min_bound.z =
                std::min(
                    min_bound.z,
                    vertex.z
                );


            max_bound.x =
                std::max(
                    max_bound.x,
                    vertex.x
                );

            max_bound.y =
                std::max(
                    max_bound.y,
                    vertex.y
                );

            max_bound.z =
                std::max(
                    max_bound.z,
                    vertex.z
                );
        }
    }


    for (
        unsigned int child_index = 0;
        child_index < node->mNumChildren;
        child_index++
    ) {

        calculate_model_bounds_recursive(
            scene,
            node->mChildren[child_index],
            current_matrix,
            min_bound,
            max_bound
        );
    }
}


// ==========================================================
// CALCULA LIMITES
// ==========================================================

static inline void calculate_model_bounds(
    GlbModel& model
) {

    model.min_bound =
        aiVector3D(
            std::numeric_limits<float>::max(),
            std::numeric_limits<float>::max(),
            std::numeric_limits<float>::max()
        );


    model.max_bound =
        aiVector3D(
            std::numeric_limits<float>::lowest(),
            std::numeric_limits<float>::lowest(),
            std::numeric_limits<float>::lowest()
        );


    aiMatrix4x4 identity;


    calculate_model_bounds_recursive(
        model.scene,
        model.scene->mRootNode,
        identity,
        model.min_bound,
        model.max_bound
    );


    model.center =
        aiVector3D(
            (
                model.min_bound.x +
                model.max_bound.x
            ) / 2.0f,

            (
                model.min_bound.y +
                model.max_bound.y
            ) / 2.0f,

            (
                model.min_bound.z +
                model.max_bound.z
            ) / 2.0f
        );


    float size_x =
        model.max_bound.x -
        model.min_bound.x;

    float size_y =
        model.max_bound.y -
        model.min_bound.y;

    float size_z =
        model.max_bound.z -
        model.min_bound.z;


    model.max_dimension =
        std::max(
            size_x,
            std::max(
                size_y,
                size_z
            )
        );


    if (
        model.max_dimension <
        0.0001f
    ) {

        model.max_dimension =
            1.0f;
    }
}


// ==========================================================
// CARREGA MODELO
// ==========================================================

static inline bool load_model(
    GlbModel& model,
    const std::string& filepath
) {

    model.importer =
        std::make_unique<Assimp::Importer>();


    model.scene =
        model.importer->ReadFile(
            filepath.c_str(),

            aiProcess_Triangulate |
            aiProcess_GenNormals |
            aiProcess_ImproveCacheLocality |
            aiProcess_JoinIdenticalVertices |
            aiProcess_CalcTangentSpace |
            aiProcess_FlipUVs
        );


    if (!model.scene) {

        std::cerr
            << "Erro ao carregar modelo: "
            << filepath
            << std::endl;

        return false;
    }


    model.textures.clear();


    for (
        unsigned int material_index = 0;
        material_index < model.scene->mNumMaterials;
        material_index++
    ) {

        const aiMaterial* material =
            model.scene->mMaterials[
                material_index
            ];


        unsigned int texture_index;


        if (
            !load_texture_index_from_material(
                texture_index,
                material
            )
        ) {

            continue;
        }


        if (
            texture_index >=
            model.scene->mNumTextures
        ) {

            continue;
        }


        GlbTexture texture;


        if (
            load_texture_embedded(
                texture,
                model.scene->mTextures[
                    texture_index
                ]
            )
        ) {

            model.textures[
                material_index
            ] = texture;
        }
    }


    calculate_model_bounds(
        model
    );


    return true;
}


// ==========================================================
// DESENHA MODELO
// ==========================================================

static inline void draw_model_recursive(
    const GlbModel& model,
    const aiNode* node
) {

    if (!node) {
        return;
    }


    glPushMatrix();


    aiMatrix4x4 matrix =
        node->mTransformation;


    matrix.Transpose();


    glMultMatrixf(
        &matrix.a1
    );


    for (
        unsigned int mesh_index = 0;
        mesh_index < node->mNumMeshes;
        mesh_index++
    ) {

        const aiMesh* mesh =
            model.scene->mMeshes[
                node->mMeshes[mesh_index]
            ];


        if (!mesh) {
            continue;
        }


        const aiMaterial* material =
            model.scene->mMaterials[
                mesh->mMaterialIndex
            ];


        // ==================================================
        // MATERIAL
        // ==================================================

        aiColor4D color(
            1.0f,
            1.0f,
            1.0f,
            1.0f
        );


        if (material) {

            if (
                aiGetMaterialColor(
                    material,
                    AI_MATKEY_BASE_COLOR,
                    &color
                ) != AI_SUCCESS
            ) {

                aiGetMaterialColor(
                    material,
                    AI_MATKEY_COLOR_DIFFUSE,
                    &color
                );
            }
        }


        glColor4f(
            color.r,
            color.g,
            color.b,
            color.a
        );


        // ==================================================
        // TEXTURA
        // ==================================================

        auto texture =
            model.textures.find(
                mesh->mMaterialIndex
            );


        if (
            texture != model.textures.end() &&
            texture->second.id != 0
        ) {

            glEnable(
                GL_TEXTURE_2D
            );


            glBindTexture(
                GL_TEXTURE_2D,
                texture->second.id
            );

        } else {

            glDisable(
                GL_TEXTURE_2D
            );
        }


        // ==================================================
        // TRIÂNGULOS
        // ==================================================

        glBegin(
            GL_TRIANGLES
        );


        for (
            unsigned int face_index = 0;
            face_index < mesh->mNumFaces;
            face_index++
        ) {

            const aiFace& face =
                mesh->mFaces[
                    face_index
                ];


            for (
                unsigned int index = 0;
                index < face.mNumIndices;
                index++
            ) {

                unsigned int vertex_index =
                    face.mIndices[index];


                if (
                    mesh->HasNormals()
                ) {

                    const aiVector3D& normal =
                        mesh->mNormals[
                            vertex_index
                        ];


                    glNormal3f(
                        normal.x,
                        normal.y,
                        normal.z
                    );
                }


                if (
                    mesh->HasTextureCoords(0)
                ) {

                    const aiVector3D& uv =
                        mesh->mTextureCoords[0][
                            vertex_index
                        ];


                    glTexCoord2f(
                        uv.x,
                        uv.y
                    );
                }


                const aiVector3D& vertex =
                    mesh->mVertices[
                        vertex_index
                    ];


                glVertex3f(
                    vertex.x,
                    vertex.y,
                    vertex.z
                );
            }
        }


        glEnd();
    }


    for (
        unsigned int child_index = 0;
        child_index < node->mNumChildren;
        child_index++
    ) {

        draw_model_recursive(
            model,
            node->mChildren[
                child_index
            ]
        );
    }


    glPopMatrix();
}


// ==========================================================
// DESENHA MODELO
// ==========================================================

static inline void draw_model(
    const GlbModel& model
) {

    if (
        !model.scene ||
        !model.scene->mRootNode
    ) {

        return;
    }


    draw_model_recursive(
        model,
        model.scene->mRootNode
    );
}


// ==========================================================
// DESENHA MODELO NORMALIZADO
// ==========================================================
//
// IMPORTANTE:
// O modelo fica centralizado em X/Z,
// mas o Y começa exatamente no chão.
//
// ==========================================================

static inline void draw_model_normalized(
    const GlbModel& model,
    float target_size
) {

    if (!model.scene) {
        return;
    }


    float scale =
        target_size /
        model.max_dimension;


    float center_x =
        (
            model.min_bound.x +
            model.max_bound.x
        ) / 2.0f;


    float center_z =
        (
            model.min_bound.z +
            model.max_bound.z
        ) / 2.0f;


    glPushMatrix();


    glScalef(
        scale,
        scale,
        scale
    );


    glTranslatef(
        -center_x,
        -model.min_bound.y,
        -center_z
    );


    draw_model(
        model
    );


    glPopMatrix();
}


// ==========================================================
// DESTROI MODELO
// ==========================================================

static inline void destroy_model(
    GlbModel& model
) {

    for (
        auto& pair :
        model.textures
    ) {

        if (
            pair.second.id != 0
        ) {

            glDeleteTextures(
                1,
                &pair.second.id
            );
        }
    }


    model.textures.clear();

    model.scene = nullptr;

    model.importer.reset();
}

#endif