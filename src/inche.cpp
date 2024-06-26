#include "inche.h"
#include "utils.h"
#include <seal/util/rlwe.h>
#include <seal/util/polyarithsmallmod.h>

using namespace seal;
using namespace seal::util;
using namespace che_utils;

namespace inche {
    Inche::Inche(scheme_type scheme, size_t poly_modulus_degree) {
        EncryptionParameters params(scheme);
        params.set_poly_modulus_degree(poly_modulus_degree);

        this->scheme = scheme;

        
        auto coeffs = CoeffModulus::BFVDefault(poly_modulus_degree);
        params.set_coeff_modulus(coeffs);
        
        // branch based on scheme type
        switch (scheme) {
            case scheme_type::ckks:
                scale = pow(2.0, log2(*(coeffs[2].data())));
                break;

            case scheme_type::bfv: case scheme_type::bgv:
                params.set_plain_modulus(16384);
                break;
        }

        // gather params
        context_ = new SEALContext(params);

        // generate keys
        KeyGenerator keygen(*context_);
        SecretKey secret_key = keygen.secret_key();
        PublicKey public_key;
        keygen.create_public_key(public_key);

        pk_ = public_key;

        // create the encryption objects
        enc  = new Encryptor(*context_, public_key);
        eval = new Evaluator(*context_);
        dec  = new Decryptor(*context_, secret_key);

        // set the encoder object, if using CKKS, then
        // encrypt the base ciphertext he(0)
        if (scheme == scheme_type::ckks) {
            Plaintext zero_plain;
            encoder = new CKKSEncoder(*context_);
            encoder->encode(0, scale, zero_plain);
            enc->encrypt(zero_plain, zero);
        } else {
            Plaintext zero_plain(uint64_to_hex_string(1));
            enc->encrypt(zero_plain, zero);
        }
    }

    void Inche::encrypt(double value, seal::Ciphertext &destination) {
        destination = zero;
        Plaintext plain;

        // ct(0) = pt(value)
        if (scheme == scheme_type::ckks) {
            encoder->encode(value, scale, plain);
            eval->add_plain_inplace(destination, plain); // takes about 5% of fresh enc
        } else {
            Plaintext plain(uint64_to_hex_string(value));
            eval->add_plain_inplace(destination, plain);
        }

        auto context = *context_;
        auto prng = UniformRandomGeneratorFactory::DefaultFactory()->create();
        auto &parms = context.get_context_data(plain.parms_id())->parms();
        auto &coeff_modulus = parms.coeff_modulus();
        size_t coeff_modulus_size = coeff_modulus.size();
        size_t coeff_count = parms.poly_modulus_degree();
        auto &context_data = *context.get_context_data(parms.parms_id());
        auto ntt_tables = context_data.small_ntt_tables();
        size_t encrypted_size = pk_.data().size();

        auto e(allocate_poly(coeff_count, coeff_modulus_size, MemoryManager::GetPool()));

        // c[j]' = c[j] + e[j] (adding noise to ciphertext)
        for (size_t j = 0; j < encrypted_size; j++) {
            SEAL_NOISE_SAMPLER(prng, parms, e.get()); // e[j] <-- R_2
            RNSIter gaussian_iter(e.get(), coeff_count); // should not be costly
            ntt_negacyclic_harvey(gaussian_iter, coeff_modulus_size, ntt_tables); // ntt(e[j]) 
            RNSIter dst_iter(destination.data(j), coeff_count); // should not be costly

            // [c[j] + e[j]] mod coeff_modulus
            add_poly_coeffmod(gaussian_iter, dst_iter, coeff_modulus_size, coeff_modulus, dst_iter); 
        }
    }

    void Inche::decrypt(seal::Ciphertext &encrypted, seal::Plaintext &destination) {
        dec->decrypt(encrypted, destination);
    }
} // namespace inche