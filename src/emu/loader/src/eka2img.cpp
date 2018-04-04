#include <loader/eka2img.h>
#include <common/log.h>
#include <miniz.h>

#include <cstdio>

namespace eka2l1 {
    namespace loader {
        struct eka2img_import_sec_header {
            uint32_t size;
            uint32_t num_reloc;
        };

        enum relocation_code {
            abs32 = 0x02,
            rel32 = 0x03,
            ldr_pc_g0 = 0x04,
            abs16 = 0x05,
            abs12 = 0x06,
            thm_abs5 = 0x07,
            abs8 = 0x08
        };

        void write_mov_abs(void* data, uint16_t sym) {
            struct inst {
                uint32_t imm12: 12;
                uint32_t ignored1: 4;
                uint32_t imm4: 4;
                uint32_t ignored2: 12;
            };

            inst* instruction = reinterpret_cast<inst*>(data);

            instruction->imm12 = sym;
            instruction->imm4 = sym >> 12;
        }

        bool relocate(void* data, relocation_code code, uint32_t s, uint32_t p, uint32_t a) {
            switch (code) {
            case abs32:
                break;
            default:
                LOG_INFO("Unimplemented relocation code: {}", (int)code);
                break;
            }

            return true;
        }

        bool relocate(void* entries, size_t size) {
            return true;
        }

        eka2img load_eka2img(const std::string& path) {
            eka2img img;

            FILE* f = fopen(path.c_str(), "rb");

            fseek(f, 0, SEEK_END);
            auto file_size = ftell(f);
            fseek(f, 0, SEEK_SET);

            fread(&img.header, 1, sizeof(eka2img_header), f);

            assert(img.header.sig == 0x434F5045);

            if (img.header.compression_type != 0) {
                fread(&img.uncompressed_size, 1, 4, f);
                std::vector<char> temp_buf(file_size - sizeof(eka2img_header) - 4);

                img.data.resize(img.uncompressed_size);

                memcpy(img.data.data(), &img.header, sizeof(eka2img_header));
                memcpy(img.data.data() + sizeof(eka2img_header), &img.uncompressed_size, 4);

                fread(temp_buf.data(), 1, temp_buf.size(), f);

                // INFLATE IT!
                mz_stream stream;
                stream.avail_in = 0;
                stream.next_in = 0;
                stream.zalloc = nullptr;
                stream.zfree = nullptr;

                if (inflateInit(&stream) != MZ_OK) {
                    LOG_ERROR("Can not intialize inflate stream");
                }

                stream.avail_in = temp_buf.size();
                stream.next_in = reinterpret_cast<const unsigned char*>(temp_buf.data());
                stream.next_out = reinterpret_cast<unsigned char*>(&img.data[sizeof(eka2img_header) + 4]);

                auto res = inflate(&stream,Z_NO_FLUSH);

                if (res != MZ_OK) {
                    LOG_ERROR("Inflate chunk failed!");
                };

                inflateEnd(&stream);

            } else {
                img.data.resize(file_size);
                fseek(f, SEEK_SET, 0);
                fread(img.data.data(), 1, img.data.size(), f);
            }

            switch (img.header.cpu) {
            case eka2_cpu::armv5:
                LOG_INFO("Detected: ARMv5");
                break;
            case eka2_cpu::armv6:
                LOG_INFO("Detected: ARMv6");
                break;
            case eka2_cpu::armv4:
                LOG_INFO("Detected: ARMv4");
                break;
            default:
                LOG_INFO("Invalid cpu specified in EKA2 Image Header. Maybe x86 or undetected");
                break;
            }

            LOG_TRACE("Code size: 0x{:x}, Text size: 0x{:x}.", img.header.code_size, img.header.text_size);

            uint32_t import_export_table_size = img.header.code_size - img.header.text_size;
            LOG_TRACE("Import + export size: 0x{:x}", import_export_table_size);
            fclose(f);

            uint32_t relocation_size, relocation_count;
            memcpy(&relocation_size, &img.data[img.header.code_reloc_offset], 4);
            memcpy(&relocation_count, &img.data[img.header.code_reloc_offset + 4], 4);

            uint8_t try1, try2, try3, try4, try5;
            memcpy(&try1, &img.data[img.header.code_reloc_offset + 5], 1);
            memcpy(&try2, &img.data[img.header.code_reloc_offset + 6], 1);
            memcpy(&try3, &img.data[img.header.code_reloc_offset + 7], 1);
            memcpy(&try4, &img.data[img.header.code_reloc_offset + 8], 1);
            memcpy(&try5, &img.data[img.header.code_reloc_offset + 9], 1);

            memcpy(&try1, &img.data[img.header.code_reloc_offset + 10], 1);
            memcpy(&try2, &img.data[img.header.code_reloc_offset + 11], 1);
            memcpy(&try3, &img.data[img.header.code_reloc_offset + 12], 1);
            memcpy(&try4, &img.data[img.header.code_reloc_offset + 13], 1);
            memcpy(&try5, &img.data[img.header.code_reloc_offset + 14], 1);

            memcpy(&try1, &img.data[img.header.code_reloc_offset + 15], 1);
            memcpy(&try2, &img.data[img.header.code_reloc_offset + 16], 1);
            memcpy(&try3, &img.data[img.header.code_reloc_offset + 17], 1);
            memcpy(&try4, &img.data[img.header.code_reloc_offset + 18], 1);
            memcpy(&try5, &img.data[img.header.code_reloc_offset + 19], 1);


            memcpy(&try1, &img.data[img.header.code_reloc_offset + 20], 1);
            memcpy(&try2, &img.data[img.header.code_reloc_offset + 21], 1);
            memcpy(&try3, &img.data[img.header.code_reloc_offset + 22], 1);
            memcpy(&try4, &img.data[img.header.code_reloc_offset + 23], 1);
            memcpy(&try5, &img.data[img.header.code_reloc_offset + 24], 1);

            return img;
        }
    }
}
