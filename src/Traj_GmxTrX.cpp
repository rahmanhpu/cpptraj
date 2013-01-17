#include <cmath>
#include "Traj_GmxTrX.h"
#include "CpptrajStdio.h"
#include "ByteRoutines.h"
#include "Constants.h" // PIOVER2

// Internal defines
#define ANGS_PER_NM 10.0 
#define ANGS2_PER_NM2 100.0

// CONSTRUCTOR
Traj_GmxTrX::Traj_GmxTrX() :
  isBigEndian_(false),
  format_(TRR),
  ir_size_(0),
  e_size_(0),
  box_size_(0),
  vir_size_(0),
  pres_size_(0),
  top_size_(0),
  sym_size_(0),
  x_size_(0),
  v_size_(0),
  f_size_(0),
  natoms_(0),
  natom3_(0),
  step_(0),
  nre_(0),
  precision_(0),
  dt_(0.0),
  lambda_(0.0),
  frameSize_(0),
  farray_(0) 
{}

// DESTRUCTOR
Traj_GmxTrX::~Traj_GmxTrX() {
  if (farray_ != 0) delete[] farray_;
}

/** For debugging, print internal info. */
void Traj_GmxTrX::GmxInfo() {
  mprintf("------------------------------\nFile ");
  Info();
  mprintf("\n\tTitle= [%s]\n", Title().c_str());
  mprintf("\tir_size= %i\n", ir_size_);
  mprintf("\te_size= %i\n", e_size_);
  mprintf("\tbox_size= %i\n", box_size_);
  mprintf("\tvir_size= %i\n", vir_size_);
  mprintf("\tpres_size= %i\n", pres_size_);
  mprintf("\ttop_size= %i\n", top_size_);
  mprintf("\tsym_size= %i\n", sym_size_);
  mprintf("\tx_size= %i\n", x_size_);
  mprintf("\tv_size= %i\n", v_size_);
  mprintf("\tf_size= %i\n", f_size_);
  mprintf("\tnatoms= %i\n", natoms_);
  mprintf("\tnatom3= %i\n", natom3_);
  mprintf("\tstep= %i\n", step_);
  mprintf("\tnre= %i\n", nre_);
  mprintf("\tprecision= %i\n", precision_);
  mprintf("\tdt= %f\n", dt_);
  mprintf("\tlambda= %f\n", lambda_);
}

//const unsigned char Traj_GmxTrX::Magic_TRR_[4] = {201, 7, 0, 0};
//const unsigned char Traj_GmxTrX::Magic_TRJ_[4] = {203, 7, 0, 0};
const int Traj_GmxTrX::Magic_ = 1993;

/** \return true if TRR/TRJ file. Determine endianness. */
bool Traj_GmxTrX::IsTRX(CpptrajFile& infile) {
  int magic;
  if ( infile.Read( &magic, 4 ) != 4 ) return 1;
  if (magic != Magic_) {
    // See if this is big endian
    endian_swap( &magic, 1 );
    if (magic != Magic_) 
      return false;
    else
      isBigEndian_ = true;
  } else
    isBigEndian_ = false;
  // TODO: At this point file is trX, but not sure how best to differentiate 
  // between TRR and TRJ. For now do it based on extension. Default TRR.
  if      (infile.Extension() == ".trr") format_ = TRR;
  else if (infile.Extension() == ".trj") format_ = TRJ;
  else format_ = TRR; 
  return true;
}

/** \return true if TRR/TRJ file. */
bool Traj_GmxTrX::ID_TrajFormat(CpptrajFile& infile) {
  // File must already be set up for read
  if (infile.OpenFile()) return false;
  bool istrx = IsTRX(infile);
  infile.CloseFile();
  return istrx;
}

// Traj_GmxTrX::closeTraj()
void Traj_GmxTrX::closeTraj() {
  file_.CloseFile();
}

/** Read 1 integer, swap bytes if big endian. */
int Traj_GmxTrX::read_int( int& ival ) {
  // ASSUMING 4 byte integers
  if ( file_.Read( &ival, 4 ) != 4 ) return 1;
  if (isBigEndian_) endian_swap( &ival, 1 );
  return 0;
}

/** Read 1 float/double based on precision, swap bytes if big endian. */
int Traj_GmxTrX::read_real( float& fval ) {
  double dval;
  switch (precision_) {
    case sizeof(float):
      if (file_.Read( &fval, precision_ ) != precision_) return 1;
      if (isBigEndian_) endian_swap( &fval, 1 );
      break;
    case sizeof(double):
      if (file_.Read( &dval, precision_ ) != precision_) return 1;
      if (isBigEndian_) endian_swap8( &dval, 1 );
      fval = (float)dval;
      break;
    default:
      return 1;
  }
  return 0;
}

/** Read an integer value which gives string size, then the following string
  * of that size.
  */
std::string Traj_GmxTrX::read_string( ) {
  int size = 0;
  // Read string size
  if ( read_int( size ) ) return std::string();
  if ( size < BUF_SIZE ) {
    // Read entire string
    file_.Read( linebuffer_, size );
    linebuffer_[size] = '\0';
    return std::string(linebuffer_);
  } else {
    // String is larger than input buffer. Read into linebuffer until
    // entire string is read.
    std::string output;
    int chunksize = BUF_SIZE - 1;
    linebuffer_[chunksize] = '\0';
    int ntimes = size / chunksize;
    for (int i = 0; i < ntimes; i++) {
      file_.Read( linebuffer_, chunksize );
      output.append( linebuffer_ );
    }
    int leftover = size % chunksize;
    // Add any leftover
    if (leftover > 0) {
      file_.Read( linebuffer_, leftover );
      linebuffer_[leftover] = '\0';
      output.append( linebuffer_ );
    }
    return output;
  }
}

int Traj_GmxTrX::ReadTrxHeader() {
  int version = 0;
  // Read past magic byte
  if (file_.Read(&version, 4) != 4) return 1;
  // Read version for TRR
  if (format_ != TRJ)
    read_int( version );
  //mprintf("DEBUG: TRX Version= %i\n", version);
  // Read in title string
  SetTitle( read_string() );
  // Read in size data
  if ( read_int( ir_size_ ) ) return 1;
  if ( read_int( e_size_ ) ) return 1;
  if ( read_int( box_size_ ) ) return 1;
  if ( read_int( vir_size_ ) ) return 1;
  if ( read_int( pres_size_ ) ) return 1;
  if ( read_int( top_size_ ) ) return 1;
  if ( read_int( sym_size_ ) ) return 1;
  if ( read_int( x_size_ ) ) return 1;
  if ( read_int( v_size_ ) ) return 1;
  if ( read_int( f_size_ ) ) return 1;
  if ( read_int( natoms_ ) ) return 1;
  if (natoms_ < 1) {
    mprinterr("Error: No atoms detected in TRX trajectory.\n");
    return 1;
  }
  natom3_ = natoms_ * 3;
  if ( read_int( step_ ) ) return 1;
  if ( read_int( nre_ ) ) return 1;
  // Determine precision
  if (x_size_ > 0)
    precision_ = x_size_ / natom3_;
  else if (v_size_ > 0)
    precision_ = v_size_ / natom3_;
  else if (f_size_ > 0)
    precision_ = f_size_ / natom3_;
  else {
    mprinterr("Error: X/V/F sizes are 0 in TRX trajectory.\n");
    return 1;
  }
  if ( precision_ != sizeof(float) &&
       precision_ != sizeof(double) )
  {
    mprinterr("Error: TRX precision %i not recognized.\n", precision_);
    return 1;
  }
  // Read timestep and lambda
  if ( read_real( dt_ ) ) return 1;
  if ( read_real( lambda_ ) ) return 1;
  return 0;
}

/** Open trX trajectory and read past header info. */
int Traj_GmxTrX::openTrajin() {
  if (file_.OpenFile()) return 1;
  ReadTrxHeader();
  GmxInfo(); // DEBUG
  return 0;
}

/** 
  * \param boxOut Double array of length 6 containing {X Y Z alpha beta gamma} 
  */
int Traj_GmxTrX::ReadBox(double* boxOut) {
  // xyz is an array of length 9 containing X{xyz} Y{xyz} Z{xyz}.
  double xyz[9];
  float f_boxIn[9];
  switch (precision_) {
    case sizeof(float):
      if (file_.Read( f_boxIn, box_size_ ) != box_size_) return 1;
      for (int i = 0; i < 9; ++i)
        xyz[i] = (double)f_boxIn[i];
      break;
    case sizeof(double):
      if (file_.Read( xyz, box_size_ ) != box_size_) return 1;
      break;
    default: return 1;
  }
  // Calculate box lengths
  boxOut[0] = sqrt((xyz[0]*xyz[0] + xyz[1]*xyz[1] + xyz[2]*xyz[2])) * ANGS_PER_NM;
  boxOut[1] = sqrt((xyz[3]*xyz[3] + xyz[4]*xyz[4] + xyz[5]*xyz[5])) * ANGS_PER_NM;
  boxOut[2] = sqrt((xyz[6]*xyz[6] + xyz[7]*xyz[7] + xyz[8]*xyz[8])) * ANGS_PER_NM;
  if (boxOut[0] <= 0.0 || boxOut[1] <= 0.0 || boxOut[2] <= 0.0) {
    // Use zero-length box size and set angles to 90
    boxOut[0] = boxOut[1] = boxOut[2] = 0.0;
    boxOut[3] = boxOut[4] = boxOut[5] = 90.0;
  } else {
    // Get angles between x+y(gamma), x+z(beta), and y+z(alpha)
    boxOut[5] = acos( (xyz[0]*xyz[3] + xyz[1]*xyz[4] + xyz[2]*xyz[5]) * 
                      ANGS2_PER_NM2 / (boxOut[0]* boxOut[1]) ) * 90.0/PIOVER2;
    boxOut[4] = acos( (xyz[0]*xyz[6] + xyz[1]*xyz[7] + xyz[2]*xyz[8]) *
                      ANGS2_PER_NM2 / (boxOut[0]* boxOut[2]) ) * 90.0/PIOVER2;
    boxOut[3] = acos( (xyz[3]*xyz[6] + xyz[4]*xyz[7] + xyz[5]*xyz[8]) *
                      ANGS2_PER_NM2 / (boxOut[1]* boxOut[2]) ) * 90.0/PIOVER2;
  }
  return 0;
}

/** Prepare trajectory for reading. Determine number of frames. */
int Traj_GmxTrX::setupTrajin(std::string const& fname, Topology* trajParm)
{
  int nframes = 0;
  if (file_.SetupRead( fname, debug_ )) return TRAJIN_ERR;
  // Call openTrajin, which will open and read past header
  if ( openTrajin() ) return TRAJIN_ERR;
  // Warn if # atoms in parm does not match
  if (trajParm->Natom() != natoms_) {
    mprinterr("Error: # atoms in TRX file (%i) does not match # atoms in parm %s (%i)\n",
              natoms_, trajParm->c_str(), trajParm->Natom());
    return TRAJIN_ERR;
  }
  // If float precision, create temp array
  if (precision_ == sizeof(float)) {
    if (farray_ != 0) delete[] farray_;
    farray_ = new float[ natom3_ ];
  }
  // Set velocity info
  SetVelocity( (v_size_ > 0) );
  // Attempt to determine # of frames in traj
  SetSeekable(false);
  size_t headerBytes = (size_t)file_.Tell();
  frameSize_ = headerBytes + (size_t)box_size_ + (size_t)vir_size_ + (size_t)pres_size_ +
                             (size_t)x_size_   + (size_t)v_size_ +   (size_t)f_size_;
                             //(size_t)ir_size_ + (size_t)e_size_ + (size_t)top_size_ + 
                             //(size_t)sym_size_;
  size_t file_size = (size_t)file_.UncompressedSize();
  if (file_size > 0) {
    nframes = (int)(file_size / frameSize_);
    if ( (file_size % frameSize_) != 0 ) {
      mprintf("Warning: %s: Number of frames in TRX file could not be accurately determined.\n",
              file_.BaseFileStr());
      mprintf("Warning: Will attempt to read %i frames.\n", nframes);
    } else
      SetSeekable(true);
  } else {
    mprintf("Warning: Uncompressed size could not be determined. This is normal for\n");
    mprintf("Warning: bzip2 files. Cannot check # of frames. Frames will be read until EOF.\n");
    nframes = TRAJIN_UNK;
  }
  // Load box info so that it can be checked.
  double box[6];
  box[0]=0.0; box[1]=0.0; box[2]=0.0; box[3]=0.0; box[4]=0.0; box[5]=0.0;
  if ( box_size_ > 0 ) {
    if ( ReadBox( box ) ) return TRAJIN_ERR;
  }
  SetBox( box ); 
  
  closeTraj();
  return nframes;
}

int Traj_GmxTrX::setupTrajout(std::string const& fname, Topology* trajParm,
                                 int NframesToWrite, bool append)
{
  return 1;
}

/** Read array of size natom3 with set precision. Swap endianness if 
  * necessary. Since GROMACS units are nm, convert to Ang.
  */
int Traj_GmxTrX::ReadAtomVector( double* Dout, int size ) {
  switch (precision_) {
    case sizeof(float):
      if (file_.Read( farray_, size ) != size) return 1;
      if (isBigEndian_) endian_swap(farray_, natom3_);
      for (int i = 0; i < natom3_; ++i)
        Dout[i] = (double)(farray_[i] * ANGS_PER_NM); // FIXME: Legit for velocities?
      break;
    case sizeof(double):
      if (file_.Read( Dout, size ) != size) return 1;
      if (isBigEndian_) endian_swap8(Dout, natom3_);
      break;
    default: return 1;
  }
  return 0;
}

int Traj_GmxTrX::readFrame(int set,double *X, double *V,double *box, double *T) {
  if (IsSeekable()) 
    file_.Seek( frameSize_ * set );
  // Read the header
  // TODO: Can we just seek past this??
  if ( ReadTrxHeader() ) return 1;
  // Read box info
  if (box_size_ > 0) {
    if (ReadBox( box )) return 1;
  }
  // Blank read past virial tensor
  if (vir_size_ > 0)
    file_.Seek( file_.Tell() + vir_size_ );
  // Blank read past pressure tensor
  if (pres_size_ > 0)
    file_.Seek( file_.Tell() + pres_size_ );
  // Read coordinates
  if (x_size_ > 0) {
    if (ReadAtomVector(X, x_size_)) {
      mprinterr("Error: Reading TRX coords frame %i\n", set+1);
      return 1;
    }
  }
  // Read velocities
  if (v_size_ > 0) {
    if (ReadAtomVector(V, v_size_)) {
      mprinterr("Error: Reading TRX velocities frame %i\n", set+1);
      return 1;
    }
  }
  // If not seekable need a blank read past forces
  if (!IsSeekable())
    file_.Seek( file_.Tell() + f_size_ );

  return 0;
}

int Traj_GmxTrX::writeFrame(int set, double *X, double *V,double *box, double T) {
  return 1;
}

void Traj_GmxTrX::Info() {
  mprintf("is a GROMACS");
   if (format_ == TRR)
    mprintf(" TRR file,");
  else
    mprintf(" TRJ file,");
  if (isBigEndian_) 
    mprintf(" big-endian");
  else
    mprintf(" little-endian");
}
