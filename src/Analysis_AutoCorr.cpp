#include "Analysis_AutoCorr.h"
#include "CpptrajStdio.h"

// CONSTRUCTOR
Analysis_AutoCorr::Analysis_AutoCorr() {}

/** Usage: autocorr [name <dsetname>] <dsetarg0> [<dsetarg1> ...] out <filename>
  */
int Analysis_AutoCorr::Setup( DataSetList* datasetlist ) {
  std::string setname_ = analyzeArgs_.GetStringKey("name");
  outfilename_ = analyzeArgs_.GetStringKey("out");
  // Select datasets
  dsets_ = datasetlist->GetMultipleSets( analyzeArgs_.GetStringNext() );
  if (dsets_.empty()) {
    mprinterr("Error: autocorr: No data sets selected.\n");
    return 1;
  }
  // Setup output datasets
  for (DataSetList::const_iterator DS = dsets_.begin(); DS != dsets_.end(); ++DS) {
    DataSet* dsout = datasetlist->AddSet( DataSet::DOUBLE, setname_, "autocorr" );
    if (dsout==NULL) return 1;
    dsout->SetLegend( (*DS)->Legend() );
    outputData_.push_back( dsout );
  }
  
  mprintf("    AUTOCORR: Calculating auto-correlation for %i data sets:\n", dsets_.size());
  dsets_.Info();
  if ( !setname_.empty() )
    mprintf("\tSet name: %s\n", setname_.c_str() );
  if ( !outfilename_.empty() )
    mprintf("\tOutfile name: %s\n", outfilename_.c_str());

  return 0;
}

int Analysis_AutoCorr::Analyze() {
  std::vector<DataSet*>::iterator dsout = outputData_.begin();
  for (DataSetList::const_iterator DS = dsets_.begin(); DS != dsets_.end(); ++DS)
  {
    (*DS)->Corr( *(*DS), *dsout, (*DS)->Size() );
    ++dsout;
  }

  return 0;
}

void Analysis_AutoCorr::Print( DataFileList* datafilelist ) {
  if (!outfilename_.empty()) {
    for (std::vector<DataSet*>::iterator dsout = outputData_.begin();
                                         dsout != outputData_.end(); ++dsout)
      datafilelist->Add( outfilename_.c_str(), *dsout );
    //DataFile* DF = datafilelist->GetDataFile( outfilename_.c_str());
    //if (DF != NULL) {
    //  //DF->ProcessArgs("xlabels " + Xlabels_);
    //  DF->ProcessArgs("ylabels " + Ylabels_);
    //}
  }
}
