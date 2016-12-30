solution "NebulaBackup"
	configurations { "Debug", "Release" }
	location(_WORKING_DIR)
	targetdir(_WORKING_DIR)

	configuration "Debug"
		flags { "Symbols" }

	configuration "Release"
		flags { "OptimizeSpeed" }

	project "Nebula"
		language "C++"
		kind "SharedLib"
		buildoptions { "-std=c++11", "-stdlib=libc++" }
		includedirs {
			".",
			"thirdparty/libressl/include"
		}
		files { 
			"libnebula/**.h",
			"libnebula/**.cpp" 
		}

		links { "ssl", "crypto" }

	project "NebulaBackup-cli"
		kind "ConsoleApp"
		language "C++"
		buildoptions { "-std=c++11", "-stdlib=libc++" }
		includedirs {
			".",
			"thirdparty/libressl/include"
		}
		files {
			"cli/*.h",
			"cli/*.cpp"
		}

		links "Nebula"

	project "crypto"
		language "C"
		kind "StaticLib"
		includedirs {
			"thirdparty/libressl/crypto",
			"thirdparty/libressl/crypto/asn1",
			"thirdparty/libressl/crypto/dsa",
			"thirdparty/libressl/crypto/evp",
			"thirdparty/libressl/crypto/modes",
			"thirdparty/libressl/include",
			"thirdparty/libressl/include/compat"
		}

		if os.is("macosx") then
			defines {
				"HAVE_STRLCPY",
				"HAVE_STRLCAT"
			}

			files {
				"thirdparty/libressl/crypto/bio/b_posix.c",
				"thirdparty/libressl/crypto/bio/bss_log.c",
				"thirdparty/libressl/crypto/ui/ui_openssl.c",
				"thirdparty/libressl/crypto/compat/reallocarray.c",
				"thirdparty/libressl/crypto/compat/timingsafe_memcmp.c",
				"thirdparty/libressl/crypto/compat/explicit_bzero.c"
			}
		end

		files {
			"thirdparty/libressl/crypto/cpt_err.c",
			"thirdparty/libressl/crypto/cryptlib.c",
			"thirdparty/libressl/crypto/cversion.c",
			"thirdparty/libressl/crypto/ex_data.c",
			"thirdparty/libressl/crypto/malloc-wrapper.c",
			"thirdparty/libressl/crypto/mem_clr.c",
			"thirdparty/libressl/crypto/mem_dbg.c",
			"thirdparty/libressl/crypto/o_init.c",
			"thirdparty/libressl/crypto/o_str.c",
			"thirdparty/libressl/crypto/o_time.c",
			"thirdparty/libressl/crypto/aes/aes_cfb.c",
			"thirdparty/libressl/crypto/aes/aes_ctr.c",
			"thirdparty/libressl/crypto/aes/aes_ecb.c",
			"thirdparty/libressl/crypto/aes/aes_ige.c",
			"thirdparty/libressl/crypto/aes/aes_misc.c",
			"thirdparty/libressl/crypto/aes/aes_ofb.c",
			"thirdparty/libressl/crypto/aes/aes_wrap.c",
			"thirdparty/libressl/crypto/asn1/a_bitstr.c",
			"thirdparty/libressl/crypto/asn1/a_bool.c",
			"thirdparty/libressl/crypto/asn1/a_bytes.c",
			"thirdparty/libressl/crypto/asn1/a_d2i_fp.c",
			"thirdparty/libressl/crypto/asn1/a_digest.c",
			"thirdparty/libressl/crypto/asn1/a_dup.c",
			"thirdparty/libressl/crypto/asn1/a_enum.c",
			"thirdparty/libressl/crypto/asn1/a_i2d_fp.c",
			"thirdparty/libressl/crypto/asn1/a_int.c",
			"thirdparty/libressl/crypto/asn1/a_mbstr.c",
			"thirdparty/libressl/crypto/asn1/a_object.c",
			"thirdparty/libressl/crypto/asn1/a_octet.c",
			"thirdparty/libressl/crypto/asn1/a_print.c",
			"thirdparty/libressl/crypto/asn1/a_set.c",
			"thirdparty/libressl/crypto/asn1/a_sign.c",
			"thirdparty/libressl/crypto/asn1/a_strex.c",
			"thirdparty/libressl/crypto/asn1/a_strnid.c",
			"thirdparty/libressl/crypto/asn1/a_time.c",
			"thirdparty/libressl/crypto/asn1/a_time_tm.c",
			"thirdparty/libressl/crypto/asn1/a_type.c",
			"thirdparty/libressl/crypto/asn1/a_utf8.c",
			"thirdparty/libressl/crypto/asn1/a_verify.c",
			"thirdparty/libressl/crypto/asn1/ameth_lib.c",
			"thirdparty/libressl/crypto/asn1/asn1_err.c",
			"thirdparty/libressl/crypto/asn1/asn1_gen.c",
			"thirdparty/libressl/crypto/asn1/asn1_lib.c",
			"thirdparty/libressl/crypto/asn1/asn1_par.c",
			"thirdparty/libressl/crypto/asn1/asn_mime.c",
			"thirdparty/libressl/crypto/asn1/asn_moid.c",
			"thirdparty/libressl/crypto/asn1/asn_pack.c",
			"thirdparty/libressl/crypto/asn1/bio_asn1.c",
			"thirdparty/libressl/crypto/asn1/bio_ndef.c",
			"thirdparty/libressl/crypto/asn1/d2i_pr.c",
			"thirdparty/libressl/crypto/asn1/d2i_pu.c",
			"thirdparty/libressl/crypto/asn1/evp_asn1.c",
			"thirdparty/libressl/crypto/asn1/f_enum.c",
			"thirdparty/libressl/crypto/asn1/f_int.c",
			"thirdparty/libressl/crypto/asn1/f_string.c",
			"thirdparty/libressl/crypto/asn1/i2d_pr.c",
			"thirdparty/libressl/crypto/asn1/i2d_pu.c",
			"thirdparty/libressl/crypto/asn1/n_pkey.c",
			"thirdparty/libressl/crypto/asn1/nsseq.c",
			"thirdparty/libressl/crypto/asn1/p5_pbe.c",
			"thirdparty/libressl/crypto/asn1/p5_pbev2.c",
			"thirdparty/libressl/crypto/asn1/p8_pkey.c",
			"thirdparty/libressl/crypto/asn1/t_bitst.c",
			"thirdparty/libressl/crypto/asn1/t_crl.c",
			"thirdparty/libressl/crypto/asn1/t_pkey.c",
			"thirdparty/libressl/crypto/asn1/t_req.c",
			"thirdparty/libressl/crypto/asn1/t_spki.c",
			"thirdparty/libressl/crypto/asn1/t_x509.c",
			"thirdparty/libressl/crypto/asn1/t_x509a.c",
			"thirdparty/libressl/crypto/asn1/tasn_dec.c",
			"thirdparty/libressl/crypto/asn1/tasn_enc.c",
			"thirdparty/libressl/crypto/asn1/tasn_fre.c",
			"thirdparty/libressl/crypto/asn1/tasn_new.c",
			"thirdparty/libressl/crypto/asn1/tasn_prn.c",
			"thirdparty/libressl/crypto/asn1/tasn_typ.c",
			"thirdparty/libressl/crypto/asn1/tasn_utl.c",
			"thirdparty/libressl/crypto/asn1/x_algor.c",
			"thirdparty/libressl/crypto/asn1/x_attrib.c",
			"thirdparty/libressl/crypto/asn1/x_bignum.c",
			"thirdparty/libressl/crypto/asn1/x_crl.c",
			"thirdparty/libressl/crypto/asn1/x_exten.c",
			"thirdparty/libressl/crypto/asn1/x_info.c",
			"thirdparty/libressl/crypto/asn1/x_long.c",
			"thirdparty/libressl/crypto/asn1/x_name.c",
			"thirdparty/libressl/crypto/asn1/x_nx509.c",
			"thirdparty/libressl/crypto/asn1/x_pkey.c",
			"thirdparty/libressl/crypto/asn1/x_pubkey.c",
			"thirdparty/libressl/crypto/asn1/x_req.c",
			"thirdparty/libressl/crypto/asn1/x_sig.c",
			"thirdparty/libressl/crypto/asn1/x_spki.c",
			"thirdparty/libressl/crypto/asn1/x_val.c",
			"thirdparty/libressl/crypto/asn1/x_x509.c",
			"thirdparty/libressl/crypto/asn1/x_x509a.c",
			"thirdparty/libressl/crypto/bf/bf_cfb64.c",
			"thirdparty/libressl/crypto/bf/bf_ecb.c",
			"thirdparty/libressl/crypto/bf/bf_enc.c",
			"thirdparty/libressl/crypto/bf/bf_ofb64.c",
			"thirdparty/libressl/crypto/bf/bf_skey.c",
			"thirdparty/libressl/crypto/bio/b_dump.c",
			"thirdparty/libressl/crypto/bio/b_print.c",
			"thirdparty/libressl/crypto/bio/b_sock.c",
			"thirdparty/libressl/crypto/bio/bf_buff.c",
			"thirdparty/libressl/crypto/bio/bf_nbio.c",
			"thirdparty/libressl/crypto/bio/bf_null.c",
			"thirdparty/libressl/crypto/bio/bio_cb.c",
			"thirdparty/libressl/crypto/bio/bio_err.c",
			"thirdparty/libressl/crypto/bio/bio_lib.c",
			"thirdparty/libressl/crypto/bio/bss_acpt.c",
			"thirdparty/libressl/crypto/bio/bss_bio.c",
			"thirdparty/libressl/crypto/bio/bss_conn.c",
			"thirdparty/libressl/crypto/bio/bss_dgram.c",
			"thirdparty/libressl/crypto/bio/bss_fd.c",
			"thirdparty/libressl/crypto/bio/bss_file.c",
			"thirdparty/libressl/crypto/bio/bss_mem.c",
			"thirdparty/libressl/crypto/bio/bss_null.c",
			"thirdparty/libressl/crypto/bio/bss_sock.c",
			"thirdparty/libressl/crypto/bn/bn_add.c",
			"thirdparty/libressl/crypto/bn/bn_asm.c",
			"thirdparty/libressl/crypto/bn/bn_blind.c",
			"thirdparty/libressl/crypto/bn/bn_const.c",
			"thirdparty/libressl/crypto/bn/bn_ctx.c",
			"thirdparty/libressl/crypto/bn/bn_depr.c",
			"thirdparty/libressl/crypto/bn/bn_div.c",
			"thirdparty/libressl/crypto/bn/bn_err.c",
			"thirdparty/libressl/crypto/bn/bn_exp.c",
			"thirdparty/libressl/crypto/bn/bn_exp2.c",
			"thirdparty/libressl/crypto/bn/bn_gcd.c",
			"thirdparty/libressl/crypto/bn/bn_gf2m.c",
			"thirdparty/libressl/crypto/bn/bn_kron.c",
			"thirdparty/libressl/crypto/bn/bn_lib.c",
			"thirdparty/libressl/crypto/bn/bn_mod.c",
			"thirdparty/libressl/crypto/bn/bn_mont.c",
			"thirdparty/libressl/crypto/bn/bn_mpi.c",
			"thirdparty/libressl/crypto/bn/bn_mul.c",
			"thirdparty/libressl/crypto/bn/bn_nist.c",
			"thirdparty/libressl/crypto/bn/bn_prime.c",
			"thirdparty/libressl/crypto/bn/bn_print.c",
			"thirdparty/libressl/crypto/bn/bn_rand.c",
			"thirdparty/libressl/crypto/bn/bn_recp.c",
			"thirdparty/libressl/crypto/bn/bn_shift.c",
			"thirdparty/libressl/crypto/bn/bn_sqr.c",
			"thirdparty/libressl/crypto/bn/bn_sqrt.c",
			"thirdparty/libressl/crypto/bn/bn_word.c",
			"thirdparty/libressl/crypto/bn/bn_x931p.c",
			"thirdparty/libressl/crypto/buffer/buf_err.c",
			"thirdparty/libressl/crypto/buffer/buf_str.c",
			"thirdparty/libressl/crypto/buffer/buffer.c",
			"thirdparty/libressl/crypto/camellia/cmll_cfb.c",
			"thirdparty/libressl/crypto/camellia/cmll_ctr.c",
			"thirdparty/libressl/crypto/camellia/cmll_ecb.c",
			"thirdparty/libressl/crypto/camellia/cmll_misc.c",
			"thirdparty/libressl/crypto/camellia/cmll_ofb.c",
			"thirdparty/libressl/crypto/cast/c_cfb64.c",
			"thirdparty/libressl/crypto/cast/c_ecb.c",
			"thirdparty/libressl/crypto/cast/c_enc.c",
			"thirdparty/libressl/crypto/cast/c_ofb64.c",
			"thirdparty/libressl/crypto/cast/c_skey.c",
			"thirdparty/libressl/crypto/chacha/chacha.c",
			"thirdparty/libressl/crypto/cmac/cm_ameth.c",
			"thirdparty/libressl/crypto/cmac/cm_pmeth.c",
			"thirdparty/libressl/crypto/cmac/cmac.c",
			"thirdparty/libressl/crypto/comp/c_rle.c",
			"thirdparty/libressl/crypto/comp/c_zlib.c",
			"thirdparty/libressl/crypto/comp/comp_err.c",
			"thirdparty/libressl/crypto/comp/comp_lib.c",
			"thirdparty/libressl/crypto/conf/conf_api.c",
			"thirdparty/libressl/crypto/conf/conf_def.c",
			"thirdparty/libressl/crypto/conf/conf_err.c",
			"thirdparty/libressl/crypto/conf/conf_lib.c",
			"thirdparty/libressl/crypto/conf/conf_mall.c",
			"thirdparty/libressl/crypto/conf/conf_mod.c",
			"thirdparty/libressl/crypto/conf/conf_sap.c",
			"thirdparty/libressl/crypto/des/cbc_cksm.c",
			"thirdparty/libressl/crypto/des/cbc_enc.c",
			"thirdparty/libressl/crypto/des/cfb64ede.c",
			"thirdparty/libressl/crypto/des/cfb64enc.c",
			"thirdparty/libressl/crypto/des/cfb_enc.c",
			"thirdparty/libressl/crypto/des/des_enc.c",
			"thirdparty/libressl/crypto/des/ecb3_enc.c",
			"thirdparty/libressl/crypto/des/ecb_enc.c",
			"thirdparty/libressl/crypto/des/ede_cbcm_enc.c",
			"thirdparty/libressl/crypto/des/enc_read.c",
			"thirdparty/libressl/crypto/des/enc_writ.c",
			"thirdparty/libressl/crypto/des/fcrypt.c",
			"thirdparty/libressl/crypto/des/fcrypt_b.c",
			"thirdparty/libressl/crypto/des/ofb64ede.c",
			"thirdparty/libressl/crypto/des/ofb64enc.c",
			"thirdparty/libressl/crypto/des/ofb_enc.c",
			"thirdparty/libressl/crypto/des/pcbc_enc.c",
			"thirdparty/libressl/crypto/des/qud_cksm.c",
			"thirdparty/libressl/crypto/des/rand_key.c",
			"thirdparty/libressl/crypto/des/set_key.c",
			"thirdparty/libressl/crypto/des/str2key.c",
			"thirdparty/libressl/crypto/des/xcbc_enc.c",
			"thirdparty/libressl/crypto/dh/dh_ameth.c",
			"thirdparty/libressl/crypto/dh/dh_asn1.c",
			"thirdparty/libressl/crypto/dh/dh_check.c",
			"thirdparty/libressl/crypto/dh/dh_depr.c",
			"thirdparty/libressl/crypto/dh/dh_err.c",
			"thirdparty/libressl/crypto/dh/dh_gen.c",
			"thirdparty/libressl/crypto/dh/dh_key.c",
			"thirdparty/libressl/crypto/dh/dh_lib.c",
			"thirdparty/libressl/crypto/dh/dh_pmeth.c",
			"thirdparty/libressl/crypto/dh/dh_prn.c",
			"thirdparty/libressl/crypto/dsa/dsa_ameth.c",
			"thirdparty/libressl/crypto/dsa/dsa_asn1.c",
			"thirdparty/libressl/crypto/dsa/dsa_depr.c",
			"thirdparty/libressl/crypto/dsa/dsa_err.c",
			"thirdparty/libressl/crypto/dsa/dsa_gen.c",
			"thirdparty/libressl/crypto/dsa/dsa_key.c",
			"thirdparty/libressl/crypto/dsa/dsa_lib.c",
			"thirdparty/libressl/crypto/dsa/dsa_ossl.c",
			"thirdparty/libressl/crypto/dsa/dsa_pmeth.c",
			"thirdparty/libressl/crypto/dsa/dsa_prn.c",
			"thirdparty/libressl/crypto/dsa/dsa_sign.c",
			"thirdparty/libressl/crypto/dsa/dsa_vrf.c",
			"thirdparty/libressl/crypto/dso/dso_dlfcn.c",
			"thirdparty/libressl/crypto/dso/dso_err.c",
			"thirdparty/libressl/crypto/dso/dso_lib.c",
			"thirdparty/libressl/crypto/dso/dso_null.c",
			"thirdparty/libressl/crypto/dso/dso_openssl.c",
			"thirdparty/libressl/crypto/ec/ec2_mult.c",
			"thirdparty/libressl/crypto/ec/ec2_oct.c",
			"thirdparty/libressl/crypto/ec/ec2_smpl.c",
			"thirdparty/libressl/crypto/ec/ec_ameth.c",
			"thirdparty/libressl/crypto/ec/ec_asn1.c",
			"thirdparty/libressl/crypto/ec/ec_check.c",
			"thirdparty/libressl/crypto/ec/ec_curve.c",
			"thirdparty/libressl/crypto/ec/ec_cvt.c",
			"thirdparty/libressl/crypto/ec/ec_err.c",
			"thirdparty/libressl/crypto/ec/ec_key.c",
			"thirdparty/libressl/crypto/ec/ec_lib.c",
			"thirdparty/libressl/crypto/ec/ec_mult.c",
			"thirdparty/libressl/crypto/ec/ec_oct.c",
			"thirdparty/libressl/crypto/ec/ec_pmeth.c",
			"thirdparty/libressl/crypto/ec/ec_print.c",
			"thirdparty/libressl/crypto/ec/eck_prn.c",
			"thirdparty/libressl/crypto/ec/ecp_mont.c",
			"thirdparty/libressl/crypto/ec/ecp_nist.c",
			"thirdparty/libressl/crypto/ec/ecp_oct.c",
			"thirdparty/libressl/crypto/ec/ecp_smpl.c",
			"thirdparty/libressl/crypto/ecdh/ech_err.c",
			"thirdparty/libressl/crypto/ecdh/ech_key.c",
			"thirdparty/libressl/crypto/ecdh/ech_lib.c",
			"thirdparty/libressl/crypto/ecdsa/ecs_asn1.c",
			"thirdparty/libressl/crypto/ecdsa/ecs_err.c",
			"thirdparty/libressl/crypto/ecdsa/ecs_lib.c",
			"thirdparty/libressl/crypto/ecdsa/ecs_ossl.c",
			"thirdparty/libressl/crypto/ecdsa/ecs_sign.c",
			"thirdparty/libressl/crypto/ecdsa/ecs_vrf.c",
			"thirdparty/libressl/crypto/engine/eng_all.c",
			"thirdparty/libressl/crypto/engine/eng_cnf.c",
			"thirdparty/libressl/crypto/engine/eng_ctrl.c",
			"thirdparty/libressl/crypto/engine/eng_dyn.c",
			"thirdparty/libressl/crypto/engine/eng_err.c",
			"thirdparty/libressl/crypto/engine/eng_fat.c",
			"thirdparty/libressl/crypto/engine/eng_init.c",
			"thirdparty/libressl/crypto/engine/eng_lib.c",
			"thirdparty/libressl/crypto/engine/eng_list.c",
			"thirdparty/libressl/crypto/engine/eng_openssl.c",
			"thirdparty/libressl/crypto/engine/eng_pkey.c",
			"thirdparty/libressl/crypto/engine/eng_table.c",
			"thirdparty/libressl/crypto/engine/tb_asnmth.c",
			"thirdparty/libressl/crypto/engine/tb_cipher.c",
			"thirdparty/libressl/crypto/engine/tb_dh.c",
			"thirdparty/libressl/crypto/engine/tb_digest.c",
			"thirdparty/libressl/crypto/engine/tb_dsa.c",
			"thirdparty/libressl/crypto/engine/tb_ecdh.c",
			"thirdparty/libressl/crypto/engine/tb_ecdsa.c",
			"thirdparty/libressl/crypto/engine/tb_pkmeth.c",
			"thirdparty/libressl/crypto/engine/tb_rand.c",
			"thirdparty/libressl/crypto/engine/tb_rsa.c",
			"thirdparty/libressl/crypto/engine/tb_store.c",
			"thirdparty/libressl/crypto/err/err.c",
			"thirdparty/libressl/crypto/err/err_all.c",
			"thirdparty/libressl/crypto/err/err_prn.c",
			"thirdparty/libressl/crypto/evp/bio_b64.c",
			"thirdparty/libressl/crypto/evp/bio_enc.c",
			"thirdparty/libressl/crypto/evp/bio_md.c",
			"thirdparty/libressl/crypto/evp/c_all.c",
			"thirdparty/libressl/crypto/evp/digest.c",
			"thirdparty/libressl/crypto/evp/e_aes.c",
			"thirdparty/libressl/crypto/evp/e_aes_cbc_hmac_sha1.c",
			"thirdparty/libressl/crypto/evp/e_bf.c",
			"thirdparty/libressl/crypto/evp/e_camellia.c",
			"thirdparty/libressl/crypto/evp/e_cast.c",
			"thirdparty/libressl/crypto/evp/e_chacha.c",
			"thirdparty/libressl/crypto/evp/e_chacha20poly1305.c",
			"thirdparty/libressl/crypto/evp/e_des.c",
			"thirdparty/libressl/crypto/evp/e_des3.c",
			"thirdparty/libressl/crypto/evp/e_gost2814789.c",
			"thirdparty/libressl/crypto/evp/e_idea.c",
			"thirdparty/libressl/crypto/evp/e_null.c",
			"thirdparty/libressl/crypto/evp/e_old.c",
			"thirdparty/libressl/crypto/evp/e_rc2.c",
			"thirdparty/libressl/crypto/evp/e_rc4.c",
			"thirdparty/libressl/crypto/evp/e_rc4_hmac_md5.c",
			"thirdparty/libressl/crypto/evp/e_xcbc_d.c",
			"thirdparty/libressl/crypto/evp/encode.c",
			"thirdparty/libressl/crypto/evp/evp_aead.c",
			"thirdparty/libressl/crypto/evp/evp_enc.c",
			"thirdparty/libressl/crypto/evp/evp_err.c",
			"thirdparty/libressl/crypto/evp/evp_key.c",
			"thirdparty/libressl/crypto/evp/evp_lib.c",
			"thirdparty/libressl/crypto/evp/evp_pbe.c",
			"thirdparty/libressl/crypto/evp/evp_pkey.c",
			"thirdparty/libressl/crypto/evp/m_dss.c",
			"thirdparty/libressl/crypto/evp/m_dss1.c",
			"thirdparty/libressl/crypto/evp/m_ecdsa.c",
			"thirdparty/libressl/crypto/evp/m_gost2814789.c",
			"thirdparty/libressl/crypto/evp/m_gostr341194.c",
			"thirdparty/libressl/crypto/evp/m_md4.c",
			"thirdparty/libressl/crypto/evp/m_md5.c",
			"thirdparty/libressl/crypto/evp/m_null.c",
			"thirdparty/libressl/crypto/evp/m_ripemd.c",
			"thirdparty/libressl/crypto/evp/m_sha1.c",
			"thirdparty/libressl/crypto/evp/m_sigver.c",
			"thirdparty/libressl/crypto/evp/m_streebog.c",
			"thirdparty/libressl/crypto/evp/m_wp.c",
			"thirdparty/libressl/crypto/evp/names.c",
			"thirdparty/libressl/crypto/evp/p5_crpt.c",
			"thirdparty/libressl/crypto/evp/p5_crpt2.c",
			"thirdparty/libressl/crypto/evp/p_dec.c",
			"thirdparty/libressl/crypto/evp/p_enc.c",
			"thirdparty/libressl/crypto/evp/p_lib.c",
			"thirdparty/libressl/crypto/evp/p_open.c",
			"thirdparty/libressl/crypto/evp/p_seal.c",
			"thirdparty/libressl/crypto/evp/p_sign.c",
			"thirdparty/libressl/crypto/evp/p_verify.c",
			"thirdparty/libressl/crypto/evp/pmeth_fn.c",
			"thirdparty/libressl/crypto/evp/pmeth_gn.c",
			"thirdparty/libressl/crypto/evp/pmeth_lib.c",
			"thirdparty/libressl/crypto/gost/gost2814789.c",
			"thirdparty/libressl/crypto/gost/gost89_keywrap.c",
			"thirdparty/libressl/crypto/gost/gost89_params.c",
			"thirdparty/libressl/crypto/gost/gost89imit_ameth.c",
			"thirdparty/libressl/crypto/gost/gost89imit_pmeth.c",
			"thirdparty/libressl/crypto/gost/gost_asn1.c",
			"thirdparty/libressl/crypto/gost/gost_err.c",
			"thirdparty/libressl/crypto/gost/gostr341001.c",
			"thirdparty/libressl/crypto/gost/gostr341001_ameth.c",
			"thirdparty/libressl/crypto/gost/gostr341001_key.c",
			"thirdparty/libressl/crypto/gost/gostr341001_params.c",
			"thirdparty/libressl/crypto/gost/gostr341001_pmeth.c",
			"thirdparty/libressl/crypto/gost/gostr341194.c",
			"thirdparty/libressl/crypto/gost/streebog.c",
			"thirdparty/libressl/crypto/hmac/hm_ameth.c",
			"thirdparty/libressl/crypto/hmac/hm_pmeth.c",
			"thirdparty/libressl/crypto/hmac/hmac.c",
			"thirdparty/libressl/crypto/idea/i_cbc.c",
			"thirdparty/libressl/crypto/idea/i_cfb64.c",
			"thirdparty/libressl/crypto/idea/i_ecb.c",
			"thirdparty/libressl/crypto/idea/i_ofb64.c",
			"thirdparty/libressl/crypto/idea/i_skey.c",
			"thirdparty/libressl/crypto/krb5/krb5_asn.c",
			"thirdparty/libressl/crypto/lhash/lh_stats.c",
			"thirdparty/libressl/crypto/lhash/lhash.c",
			"thirdparty/libressl/crypto/md4/md4_dgst.c",
			"thirdparty/libressl/crypto/md4/md4_one.c",
			"thirdparty/libressl/crypto/md5/md5_dgst.c",
			"thirdparty/libressl/crypto/md5/md5_one.c",
			"thirdparty/libressl/crypto/modes/cbc128.c",
			"thirdparty/libressl/crypto/modes/ccm128.c",
			"thirdparty/libressl/crypto/modes/cfb128.c",
			"thirdparty/libressl/crypto/modes/ctr128.c",
			"thirdparty/libressl/crypto/modes/cts128.c",
			"thirdparty/libressl/crypto/modes/gcm128.c",
			"thirdparty/libressl/crypto/modes/ofb128.c",
			"thirdparty/libressl/crypto/modes/xts128.c",
			"thirdparty/libressl/crypto/objects/o_names.c",
			"thirdparty/libressl/crypto/objects/obj_dat.c",
			"thirdparty/libressl/crypto/objects/obj_err.c",
			"thirdparty/libressl/crypto/objects/obj_lib.c",
			"thirdparty/libressl/crypto/objects/obj_xref.c",
			"thirdparty/libressl/crypto/ocsp/ocsp_asn.c",
			"thirdparty/libressl/crypto/ocsp/ocsp_cl.c",
			"thirdparty/libressl/crypto/ocsp/ocsp_err.c",
			"thirdparty/libressl/crypto/ocsp/ocsp_ext.c",
			"thirdparty/libressl/crypto/ocsp/ocsp_ht.c",
			"thirdparty/libressl/crypto/ocsp/ocsp_lib.c",
			"thirdparty/libressl/crypto/ocsp/ocsp_prn.c",
			"thirdparty/libressl/crypto/ocsp/ocsp_srv.c",
			"thirdparty/libressl/crypto/ocsp/ocsp_vfy.c",
			"thirdparty/libressl/crypto/pem/pem_all.c",
			"thirdparty/libressl/crypto/pem/pem_err.c",
			"thirdparty/libressl/crypto/pem/pem_info.c",
			"thirdparty/libressl/crypto/pem/pem_lib.c",
			"thirdparty/libressl/crypto/pem/pem_oth.c",
			"thirdparty/libressl/crypto/pem/pem_pk8.c",
			"thirdparty/libressl/crypto/pem/pem_pkey.c",
			"thirdparty/libressl/crypto/pem/pem_seal.c",
			"thirdparty/libressl/crypto/pem/pem_sign.c",
			"thirdparty/libressl/crypto/pem/pem_x509.c",
			"thirdparty/libressl/crypto/pem/pem_xaux.c",
			"thirdparty/libressl/crypto/pem/pvkfmt.c",
			"thirdparty/libressl/crypto/pkcs12/p12_add.c",
			"thirdparty/libressl/crypto/pkcs12/p12_asn.c",
			"thirdparty/libressl/crypto/pkcs12/p12_attr.c",
			"thirdparty/libressl/crypto/pkcs12/p12_crpt.c",
			"thirdparty/libressl/crypto/pkcs12/p12_crt.c",
			"thirdparty/libressl/crypto/pkcs12/p12_decr.c",
			"thirdparty/libressl/crypto/pkcs12/p12_init.c",
			"thirdparty/libressl/crypto/pkcs12/p12_key.c",
			"thirdparty/libressl/crypto/pkcs12/p12_kiss.c",
			"thirdparty/libressl/crypto/pkcs12/p12_mutl.c",
			"thirdparty/libressl/crypto/pkcs12/p12_npas.c",
			"thirdparty/libressl/crypto/pkcs12/p12_p8d.c",
			"thirdparty/libressl/crypto/pkcs12/p12_p8e.c",
			"thirdparty/libressl/crypto/pkcs12/p12_utl.c",
			"thirdparty/libressl/crypto/pkcs12/pk12err.c",
			"thirdparty/libressl/crypto/pkcs7/bio_pk7.c",
			"thirdparty/libressl/crypto/pkcs7/pk7_asn1.c",
			"thirdparty/libressl/crypto/pkcs7/pk7_attr.c",
			"thirdparty/libressl/crypto/pkcs7/pk7_doit.c",
			"thirdparty/libressl/crypto/pkcs7/pk7_lib.c",
			"thirdparty/libressl/crypto/pkcs7/pk7_mime.c",
			"thirdparty/libressl/crypto/pkcs7/pk7_smime.c",
			"thirdparty/libressl/crypto/pkcs7/pkcs7err.c",
			"thirdparty/libressl/crypto/poly1305/poly1305.c",
			"thirdparty/libressl/crypto/rand/rand_err.c",
			"thirdparty/libressl/crypto/rand/rand_lib.c",
			"thirdparty/libressl/crypto/rand/randfile.c",
			"thirdparty/libressl/crypto/rc2/rc2_cbc.c",
			"thirdparty/libressl/crypto/rc2/rc2_ecb.c",
			"thirdparty/libressl/crypto/rc2/rc2_skey.c",
			"thirdparty/libressl/crypto/rc2/rc2cfb64.c",
			"thirdparty/libressl/crypto/rc2/rc2ofb64.c",
			"thirdparty/libressl/crypto/ripemd/rmd_dgst.c",
			"thirdparty/libressl/crypto/ripemd/rmd_one.c",
			"thirdparty/libressl/crypto/rsa/rsa_ameth.c",
			"thirdparty/libressl/crypto/rsa/rsa_asn1.c",
			"thirdparty/libressl/crypto/rsa/rsa_chk.c",
			"thirdparty/libressl/crypto/rsa/rsa_crpt.c",
			"thirdparty/libressl/crypto/rsa/rsa_depr.c",
			"thirdparty/libressl/crypto/rsa/rsa_eay.c",
			"thirdparty/libressl/crypto/rsa/rsa_err.c",
			"thirdparty/libressl/crypto/rsa/rsa_gen.c",
			"thirdparty/libressl/crypto/rsa/rsa_lib.c",
			"thirdparty/libressl/crypto/rsa/rsa_none.c",
			"thirdparty/libressl/crypto/rsa/rsa_oaep.c",
			"thirdparty/libressl/crypto/rsa/rsa_pk1.c",
			"thirdparty/libressl/crypto/rsa/rsa_pmeth.c",
			"thirdparty/libressl/crypto/rsa/rsa_prn.c",
			"thirdparty/libressl/crypto/rsa/rsa_pss.c",
			"thirdparty/libressl/crypto/rsa/rsa_saos.c",
			"thirdparty/libressl/crypto/rsa/rsa_sign.c",
			"thirdparty/libressl/crypto/rsa/rsa_ssl.c",
			"thirdparty/libressl/crypto/rsa/rsa_x931.c",
			"thirdparty/libressl/crypto/sha/sha1_one.c",
			"thirdparty/libressl/crypto/sha/sha1dgst.c",
			"thirdparty/libressl/crypto/sha/sha256.c",
			"thirdparty/libressl/crypto/sha/sha512.c",
			"thirdparty/libressl/crypto/stack/stack.c",
			"thirdparty/libressl/crypto/ts/ts_asn1.c",
			"thirdparty/libressl/crypto/ts/ts_conf.c",
			"thirdparty/libressl/crypto/ts/ts_err.c",
			"thirdparty/libressl/crypto/ts/ts_lib.c",
			"thirdparty/libressl/crypto/ts/ts_req_print.c",
			"thirdparty/libressl/crypto/ts/ts_req_utils.c",
			"thirdparty/libressl/crypto/ts/ts_rsp_print.c",
			"thirdparty/libressl/crypto/ts/ts_rsp_sign.c",
			"thirdparty/libressl/crypto/ts/ts_rsp_utils.c",
			"thirdparty/libressl/crypto/ts/ts_rsp_verify.c",
			"thirdparty/libressl/crypto/ts/ts_verify_ctx.c",
			"thirdparty/libressl/crypto/txt_db/txt_db.c",
			"thirdparty/libressl/crypto/ui/ui_err.c",
			"thirdparty/libressl/crypto/ui/ui_lib.c",
			"thirdparty/libressl/crypto/ui/ui_util.c",
			"thirdparty/libressl/crypto/whrlpool/wp_dgst.c",
			"thirdparty/libressl/crypto/x509/by_dir.c",
			"thirdparty/libressl/crypto/x509/by_file.c",
			"thirdparty/libressl/crypto/x509/by_mem.c",
			"thirdparty/libressl/crypto/x509/x509_att.c",
			"thirdparty/libressl/crypto/x509/x509_cmp.c",
			"thirdparty/libressl/crypto/x509/x509_d2.c",
			"thirdparty/libressl/crypto/x509/x509_def.c",
			"thirdparty/libressl/crypto/x509/x509_err.c",
			"thirdparty/libressl/crypto/x509/x509_ext.c",
			"thirdparty/libressl/crypto/x509/x509_lu.c",
			"thirdparty/libressl/crypto/x509/x509_obj.c",
			"thirdparty/libressl/crypto/x509/x509_r2x.c",
			"thirdparty/libressl/crypto/x509/x509_req.c",
			"thirdparty/libressl/crypto/x509/x509_set.c",
			"thirdparty/libressl/crypto/x509/x509_trs.c",
			"thirdparty/libressl/crypto/x509/x509_txt.c",
			"thirdparty/libressl/crypto/x509/x509_v3.c",
			"thirdparty/libressl/crypto/x509/x509_vfy.c",
			"thirdparty/libressl/crypto/x509/x509_vpm.c",
			"thirdparty/libressl/crypto/x509/x509cset.c",
			"thirdparty/libressl/crypto/x509/x509name.c",
			"thirdparty/libressl/crypto/x509/x509rset.c",
			"thirdparty/libressl/crypto/x509/x509spki.c",
			"thirdparty/libressl/crypto/x509/x509type.c",
			"thirdparty/libressl/crypto/x509/x_all.c",
			"thirdparty/libressl/crypto/x509v3/pcy_cache.c",
			"thirdparty/libressl/crypto/x509v3/pcy_data.c",
			"thirdparty/libressl/crypto/x509v3/pcy_lib.c",
			"thirdparty/libressl/crypto/x509v3/pcy_map.c",
			"thirdparty/libressl/crypto/x509v3/pcy_node.c",
			"thirdparty/libressl/crypto/x509v3/pcy_tree.c",
			"thirdparty/libressl/crypto/x509v3/v3_akey.c",
			"thirdparty/libressl/crypto/x509v3/v3_akeya.c",
			"thirdparty/libressl/crypto/x509v3/v3_alt.c",
			"thirdparty/libressl/crypto/x509v3/v3_bcons.c",
			"thirdparty/libressl/crypto/x509v3/v3_bitst.c",
			"thirdparty/libressl/crypto/x509v3/v3_conf.c",
			"thirdparty/libressl/crypto/x509v3/v3_cpols.c",
			"thirdparty/libressl/crypto/x509v3/v3_crld.c",
			"thirdparty/libressl/crypto/x509v3/v3_enum.c",
			"thirdparty/libressl/crypto/x509v3/v3_extku.c",
			"thirdparty/libressl/crypto/x509v3/v3_genn.c",
			"thirdparty/libressl/crypto/x509v3/v3_ia5.c",
			"thirdparty/libressl/crypto/x509v3/v3_info.c",
			"thirdparty/libressl/crypto/x509v3/v3_int.c",
			"thirdparty/libressl/crypto/x509v3/v3_lib.c",
			"thirdparty/libressl/crypto/x509v3/v3_ncons.c",
			"thirdparty/libressl/crypto/x509v3/v3_ocsp.c",
			"thirdparty/libressl/crypto/x509v3/v3_pci.c",
			"thirdparty/libressl/crypto/x509v3/v3_pcia.c",
			"thirdparty/libressl/crypto/x509v3/v3_pcons.c",
			"thirdparty/libressl/crypto/x509v3/v3_pku.c",
			"thirdparty/libressl/crypto/x509v3/v3_pmaps.c",
			"thirdparty/libressl/crypto/x509v3/v3_prn.c",
			"thirdparty/libressl/crypto/x509v3/v3_purp.c",
			"thirdparty/libressl/crypto/x509v3/v3_skey.c",
			"thirdparty/libressl/crypto/x509v3/v3_sxnet.c",
			"thirdparty/libressl/crypto/x509v3/v3_utl.c",
			"thirdparty/libressl/crypto/x509v3/v3err.c"
		}

		files {
			"thirdparty/libressl/crypto/aes/aes_cbc.c",
			"thirdparty/libressl/crypto/aes/aes_core.c",
			"thirdparty/libressl/crypto/camellia/camellia.c",
			"thirdparty/libressl/crypto/camellia/cmll_cbc.c",
			"thirdparty/libressl/crypto/rc4/rc4_enc.c",
			"thirdparty/libressl/crypto/rc4/rc4_skey.c",
			"thirdparty/libressl/crypto/whrlpool/wp_block.c"
		}

	project "ssl"
		language "C"
		kind "SharedLib"
		includedirs {
			"thirdparty/libressl/include",
			"thirdparty/libressl/include/compat",
		}

		if os.is("macosx") then
			defines {
				"HAVE_STRLCPY",
				"HAVE_STRLCAT",
			}
		end

		files {
			"thirdparty/libressl/ssl/bio_ssl.c",
			"thirdparty/libressl/ssl/bs_ber.c",
			"thirdparty/libressl/ssl/bs_cbb.c",
			"thirdparty/libressl/ssl/bs_cbs.c",
			"thirdparty/libressl/ssl/d1_both.c",
			"thirdparty/libressl/ssl/d1_clnt.c",
			"thirdparty/libressl/ssl/d1_enc.c",
			"thirdparty/libressl/ssl/d1_lib.c",
			"thirdparty/libressl/ssl/d1_meth.c",
			"thirdparty/libressl/ssl/d1_pkt.c",
			"thirdparty/libressl/ssl/d1_srtp.c",
			"thirdparty/libressl/ssl/d1_srvr.c",
			"thirdparty/libressl/ssl/pqueue.c",
			"thirdparty/libressl/ssl/s23_clnt.c",
			"thirdparty/libressl/ssl/s23_lib.c",
			"thirdparty/libressl/ssl/s23_pkt.c",
			"thirdparty/libressl/ssl/s23_srvr.c",
			"thirdparty/libressl/ssl/s3_both.c",
			"thirdparty/libressl/ssl/s3_cbc.c",
			"thirdparty/libressl/ssl/s3_clnt.c",
			"thirdparty/libressl/ssl/s3_lib.c",
			"thirdparty/libressl/ssl/s3_pkt.c",
			"thirdparty/libressl/ssl/s3_srvr.c",
			"thirdparty/libressl/ssl/ssl_algs.c",
			"thirdparty/libressl/ssl/ssl_asn1.c",
			"thirdparty/libressl/ssl/ssl_cert.c",
			"thirdparty/libressl/ssl/ssl_ciph.c",
			"thirdparty/libressl/ssl/ssl_err.c",
			"thirdparty/libressl/ssl/ssl_err2.c",
			"thirdparty/libressl/ssl/ssl_lib.c",
			"thirdparty/libressl/ssl/ssl_rsa.c",
			"thirdparty/libressl/ssl/ssl_sess.c",
			"thirdparty/libressl/ssl/ssl_stat.c",
			"thirdparty/libressl/ssl/ssl_txt.c",
			"thirdparty/libressl/ssl/t1_clnt.c",
			"thirdparty/libressl/ssl/t1_enc.c",
			"thirdparty/libressl/ssl/t1_lib.c",
			"thirdparty/libressl/ssl/t1_meth.c",
			"thirdparty/libressl/ssl/t1_reneg.c",
			"thirdparty/libressl/ssl/t1_srvr.c"

		}
		links { "crypto" }