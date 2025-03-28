#include "data/lattice/lattice.h"

#include <cassert>
#include <cstring>
#include <vector>
#include <istream>
#include <ostream>
#include <sstream>
#include <cmath>

namespace flatter {

Lattice::Lattice() :
    Lattice(0, 0)
{}

Lattice::Lattice(unsigned int nvecs, unsigned int dimension) {
    this->B = Matrix(ElementType::MPZ, dimension, nvecs);
    this->rank_ = std::min(dimension, nvecs);
    profile = Profile(this->rank_);
}

Lattice::Lattice(Matrix B) {
    this->B = B;
    this->rank_ = std::min(B.nrows(), B.ncols());
    profile = Profile(this->rank_);
}

void Lattice::resize(unsigned int nvecs, unsigned int dimension) {
    B = Matrix(ElementType::MPZ, dimension, nvecs);
    rank_ = std::min(dimension, nvecs);
    profile = Profile(rank_);
}

void Lattice::update_rank() {
    MatrixData<mpz_t> dB = B.data<mpz_t>();
    for (unsigned int i = 0; i < B.ncols(); i++) {
        unsigned int col = B.ncols() - 1 - i;
        // if all entries in the column are 0, decrement the rank
        unsigned int j;
        for (j = 0; j < B.nrows(); j++) {
            if (mpz_sgn(dB(j,col)) != 0) {
                break;
            }
        }
        if (j == B.nrows()) {
            // all entries are 0
            rank_ = col;
        }
    }
    rank_ = std::min(rank_, B.nrows());
    Profile new_profile = Profile(rank_);
    for (unsigned int i = 0; i < rank_; i++) {
        new_profile[i] = profile[i];
    }
    profile = new_profile;
}

Matrix Lattice::basis() const {
    return B;
}

Matrix& Lattice::basis() {
    return B;
}

unsigned int Lattice::rank() const {
    return rank_;
}

unsigned int Lattice::dimension() const {
    return B.nrows();
}

std::vector<std::string> parse_line(std::string line) {
  assert(line[line.length() - 1] == ']');

  while (line[line.length() - 1] == ']' || line[line.length() - 1] == ' ') {
    line.pop_back();
  }

  std::istringstream iss(line);
  std::string f;
  std::vector<std::string> v;

  while (!iss.eof()) {
    iss >> f;
    v.push_back(f);
  }
  return v;
}

std::istream& operator>>(std::istream& is, Lattice& L) {
    char c;
    std::string line;
    unsigned int rank;
    unsigned int dim;

    // Do a single pass to get lattice dimensions and bits required
    std::vector<std::vector<std::string>> data;

    is >> c;
    if (c != '[') {
        throw "Invalid Lattice";
    }
    while (!is.eof()) {
        is >> c;
        if (is.eof()) {
            break;
        }
        if (c == '[') {
            std::getline(is, line);
            auto row = parse_line(line);
            data.push_back(row);
        } else if (c == ']') {
            break;
        } else {
            throw "Invalid Lattice";
        }
    }

    // The rounded data is now in the std::vectors
    rank = data.size();
    dim = data[0].size();
    
    L.resize(rank, dim);
    flatter::MatrixData<mpz_t> dM = L.basis().data<mpz_t>();

    for (unsigned int i = 0; i < dim; i++) {
        for (unsigned int j = 0; j < rank; j++) {
            mpz_set_str(dM(i, j), data[j][i].c_str(), 0);
        }
    }

    return is;
}

std::ostream& operator<<(std::ostream& os, Lattice& L) {
    unsigned int rank = L.rank();
    unsigned int dim = L.dimension();

    MatrixData<mpz_t> dM = L.basis().data<mpz_t>();

    void (*free)(void *, size_t);
    mp_get_memory_functions (NULL, NULL, &free);

    os << "[";
    for (unsigned int i = 0; i < rank; i++) {
        os << "[";
        for (unsigned int j = 0; j < dim; j++) {
            char* elem = mpz_get_str(nullptr, 10, dM(j, i));
            std::string elem_s(elem);
            os << elem_s;
            if (j < dim - 1) {
                os << " ";
            } else {
                os << "]" << std::endl;
            }

            free(elem, strlen(elem) + 1);
        }
    }
    os << "]" << std::endl;
    return os;
}

}