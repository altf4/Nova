//============================================================================
// Name        : ClassificationEngine.h
// Copyright   : DataSoft Corporation 2011-2012
//	Nova is free software: you can redistribute it and/or modify
//   it under the terms of the GNU General Public License as published by
//   the Free Software Foundation, either version 3 of the License, or
//   (at your option) any later version.
//
//   Nova is distributed in the hope that it will be useful,
//   but WITHOUT ANY WARRANTY; without even the implied warranty of
//   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//   GNU General Public License for more details.
//
//   You should have received a copy of the GNU General Public License
//   along with Nova.  If not, see <http://www.gnu.org/licenses/>.
// Description : Suspect classification engine
//============================================================================

#include "ClassificationEngine.h"

#include <boost/format.hpp>

using namespace std;
using boost::format;

// Normalization method to use on each feature
// TODO: Make this a configuration var somewhere in Novaconfig.txt?
normalizationType ClassificationEngine::m_normalization[] = {
		LINEAR_SHIFT, // Don't normalize IP traffic distribution, already between 0 and 1
		LINEAR_SHIFT,
		LOGARITHMIC,
		LOGARITHMIC,
		LOGARITHMIC,
		LOGARITHMIC,
		LOGARITHMIC,
		LOGARITHMIC,
		LOGARITHMIC
};

ClassificationEngine::ClassificationEngine(SuspectTable &table)
: m_suspects(table)
{
	m_normalizedDataPts = NULL;
	m_dataPts = NULL;
}

ClassificationEngine::~ClassificationEngine()
{

}

void ClassificationEngine::FormKdTree()
{
	delete m_kdTree;
	//Normalize the data points
	//Foreach data point
	for(uint j = 0;j < Config::Inst()->getEnabledFeatureCount();j++)
	{
		//Foreach feature within the data point
		for(int i=0;i < m_nPts;i++)
		{
			if(m_maxFeatureValues[j] != 0)
			{
				m_normalizedDataPts[i][j] = Normalize(m_normalization[j], m_dataPts[i][j], m_minFeatureValues[j], m_maxFeatureValues[j]);
			}
			else
			{
				LOG(ERROR, (format("File %1% at line %2%: The max value of a feature was 0. Is the training data file corrupt or missing?")%__LINE__%__FILE__).str());
				break;
			}
		}
	}
	m_kdTree = new ANNkd_tree(					// build search structure
			m_normalizedDataPts,					// the data points
					m_nPts,						// number of points
					Config::Inst()->getEnabledFeatureCount());						// dimension of space
}


void ClassificationEngine::Classify(Suspect *suspect)
{
	int k = Config::Inst()->getK();
	double d;
	ANNidxArray nnIdx = new ANNidx[k];			// allocate near neigh indices
	ANNdistArray dists = new ANNdist[k];		// allocate near neighbor dists
	ANNpoint aNN = suspect->GetAnnPoint();
	featureIndex fi;

	m_kdTree->annkSearch(							// search
			aNN,								// query point
			k,									// number of near neighbors
			nnIdx,								// nearest neighbors (returned)
			dists,								// distance (returned)
			Config::Inst()->getEps());								// error bound

	for(int i = 0; i < DIM; i++)
	{
		fi = (featureIndex)i;
		suspect->SetFeatureAccuracy(fi, 0);
	}

	suspect->SetHostileNeighbors(0);

	//Determine classification according to weight by distance
	//	.5 + E[(1-Dist) * Class] / 2k (Where Class is -1 or 1)
	//	This will make the classification range from 0 to 1
	double classifyCount = 0;

	for(int i = 0; i < k; i++)
	{
		dists[i] = sqrt(dists[i]);				// unsquare distance
		for(int j = 0; j < DIM; j++)
		{
			if (Config::Inst()->isFeatureEnabled(j))
			{
				double distance = (aNN[j] - m_kdTree->thePoints()[nnIdx[i]][j]);

				if (distance < 0)
				{
					distance *= -1;
				}

				fi = (featureIndex)j;
				d  = suspect->GetFeatureAccuracy(fi) + distance;
				suspect->SetFeatureAccuracy(fi, d);
			}
		}

		if(nnIdx[i] == -1)
		{
			LOG(ERROR, (format("File %1% at line %2%: Unable to find a nearest neighbor for Data point %3% Try decreasing the Error bound")
					%__LINE__%__FILE__%i).str());
		}
		else
		{
			//If Hostile
			if(m_dataPtsWithClass[nnIdx[i]]->m_classification == 1)
			{
				classifyCount += (sqrt(DIM) - dists[i]);
				suspect->SetHostileNeighbors(suspect->GetHostileNeighbors()+1);
			}
			//If benign
			else if(m_dataPtsWithClass[nnIdx[i]]->m_classification == 0)
			{
				classifyCount -= (sqrt(DIM) - dists[i]);
			}
			else
			{
				//error case; Data points must be 0 or 1
				LOG(ERROR, (format("File %1% at line %2%: Data point has invalid classification. Should by 0 or 1, but is %3%")
						%__LINE__%__FILE__%m_dataPtsWithClass[nnIdx[i]]->m_classification).str());

				suspect->SetClassification(-1);
				delete [] nnIdx;							// clean things up
				delete [] dists;
				annClose();
				return;
			}
		}
	}
	for(int j = 0; j < DIM; j++)
	{
		fi = (featureIndex)j;
		d = suspect->GetFeatureAccuracy(fi) / k;
		suspect->SetFeatureAccuracy(fi, d);
	}


	suspect->SetClassification(.5 + (classifyCount / ((2.0 * (double)k) * sqrt(DIM) )));

	// Fix for rounding errors caused by double's not being precise enough if DIM is something like 2
	if (suspect->GetClassification() < 0)
		suspect->SetClassification(0);
	else if (suspect->GetClassification() > 1)
		suspect->SetClassification(1);

	if( suspect->GetClassification() > Config::Inst()->getClassificationThreshold())
	{
		suspect->SetIsHostile(true);
	}
	else
	{
		suspect->SetIsHostile(false);
	}
	delete [] nnIdx;							// clean things up
    delete [] dists;

    annClose();
	suspect->SetNeedsClassificationUpdate(false);
}


void ClassificationEngine::NormalizeDataPoints()
{
	for(SuspectTableIterator it = m_suspects.Begin(); it.GetIndex() != m_suspects.Size(); ++it)
	{
		// Used for matching the 0->DIM index with the 0->Config::Inst()->getEnabledFeatureCount() index
		int ai = 0;
		cout << "Key : Index ( " << it.GetKey() << " : " << it.GetIndex() << " )"<< endl;
		cout << "Suspect is " << it.Current().ToString() << endl;
		FeatureSet fs = it.Current().GetFeatureSet();
		for(int i = 0;i < DIM;i++)
		{
			if (Config::Inst()->isFeatureEnabled(i))
			{
				if(fs.m_features[i] > m_maxFeatureValues[ai])
				{
					fs.m_features[i] = m_maxFeatureValues[ai];
					//For proper normalization the upper bound for a feature is the max value of the data.
				}
				else if (fs.m_features[i] < m_minFeatureValues[ai])
				{
					fs.m_features[i] = m_minFeatureValues[ai];
				}
				ai++;
			}
		}
		m_suspects[it.GetKey()].SetFeatureSet(&fs);
	}

	//Normalize the suspect points
	for(SuspectTableIterator it = m_suspects.Begin(); it.GetIndex() != m_suspects.Size(); ++it)
	{
		if(it.Current().GetNeedsFeatureUpdate())
		{
			int ai = 0;
			ANNpoint aNN = it.Current().GetAnnPoint();
			if(aNN == NULL)
			{
				m_suspects[it.GetKey()].SetAnnPoint(annAllocPt(Config::Inst()->getEnabledFeatureCount()));
				aNN = it.Current().GetAnnPoint();
			}

			for(int i = 0; i < DIM; i++)
			{
				if (Config::Inst()->isFeatureEnabled(i))
				{
					if(m_maxFeatureValues[ai] != 0)
					{
						aNN[ai] = Normalize(m_normalization[i], it.Current().GetFeatureSet().m_features[i], m_minFeatureValues[ai], m_maxFeatureValues[ai]);
					}
					else
					{
						LOG(ERROR, (format("File %1% at line %2%: Max value for a feature is 0. Normalization failed. Is the training data corrupt or missing?")
								%__LINE__%__FILE__).str());
					}
					ai++;
				}
			}
			m_suspects[it.GetKey()].SetAnnPoint(aNN);
			m_suspects[it.GetKey()].SetNeedsFeatureUpdate(false);
		}
	}
}


void ClassificationEngine::PrintPt(ostream &out, ANNpoint p)
{
	out << "(" << p[0];
	for(uint i = 1;i < Config::Inst()->getEnabledFeatureCount();i++)
	{
		out << ", " << p[i];
	}
	out << ")\n";
}


void ClassificationEngine::LoadDataPointsFromFile(string inFilePath)
{
	ifstream myfile (inFilePath.data());
	string line;

		// Clear max and min values
		for (int i = 0; i < DIM; i++)
			m_maxFeatureValues[i] = 0;

		for (int i = 0; i < DIM; i++)
			m_minFeatureValues[i] = 0;

		for (int i = 0; i < DIM; i++)
			m_meanFeatureValues[i] = 0;

		// Reload the data file
		if (m_dataPts != NULL)
			annDeallocPts(m_dataPts);
		if (m_normalizedDataPts != NULL)
			annDeallocPts(m_normalizedDataPts);

		m_dataPtsWithClass.clear();

	//string array to check whether a line in data.txt file has the right number of fields
	string fieldsCheck[DIM];
	bool valid = true;

	int i = 0;
	int k = 0;
	int badLines = 0;

	//Count the number of data points for allocation
	if (myfile.is_open())
	{
		while (!myfile.eof())
		{
			if(myfile.peek() == EOF)
			{
				break;
			}
			getline(myfile,line);
			i++;
		}
	}

	else
	{
		LOG(ERROR, (format("File %1% at line %2%: Unable to open the training data file at %3%")%__LINE__%__FILE__%Config::Inst()->getPathTrainingFile()).str());
	}

	myfile.close();
	int maxPts = i;

	//Open the file again, allocate the number of points and assign
	myfile.open(inFilePath.data(), ifstream::in);
	m_dataPts = annAllocPts(maxPts, Config::Inst()->getEnabledFeatureCount()); // allocate data points
	m_normalizedDataPts = annAllocPts(maxPts, Config::Inst()->getEnabledFeatureCount());


	if (myfile.is_open())
	{
		i = 0;

		while (!myfile.eof() && (i < maxPts))
		{
			k = 0;

			if(myfile.peek() == EOF)
			{
				break;
			}

			//initializes fieldsCheck to have all array indices contain the string "NotPresent"
			for(int j = 0; j < DIM; j++)
			{
				fieldsCheck[j] = "NotPresent";
			}

			//this will grab a line of values up to a newline or until DIM values have been taken in.
			while(myfile.peek() != '\n' && k < DIM)
			{
				getline(myfile, fieldsCheck[k], ' ');
				k++;
			}


			//starting from the end of fieldsCheck, if NotPresent is still inside the array, then
			//the line of the data.txt file is incorrect, set valid to false. Note that this
			//only works in regards to the 9 data points preceding the classification,
			//not the classification itself.
			for(int m = DIM - 1; m >= 0 && valid; m--)
			{
				if(!fieldsCheck[m].compare("NotPresent"))
				{
					valid = false;
				}
			}

			//if the next character is a newline after extracting as many data points as possible,
			//then the classification is not present. For now, I will merely discard the line;
			//there may be a more elegant way to do it. (i.e. pass the data to Classify or something)
			if(myfile.peek() == '\n' || myfile.peek() == ' ')
			{
				valid = false;
			}

			//if the line is valid, continue as normal
			if(valid)
			{
				m_dataPtsWithClass.push_back(new Point(Config::Inst()->getEnabledFeatureCount()));

				// Used for matching the 0->DIM index with the 0->Config::Inst()->getEnabledFeatureCount() index
				int actualDimension = 0;
				for(int defaultDimension = 0;defaultDimension < DIM;defaultDimension++)
				{
					double temp = strtod(fieldsCheck[defaultDimension].data(), NULL);

					if(Config::Inst()->isFeatureEnabled(defaultDimension))
					{
						m_dataPtsWithClass[i]->m_annPoint[actualDimension] = temp;
						m_dataPts[i][actualDimension] = temp;

						//Set the max values of each feature. (Used later in normalization)
						if(temp > m_maxFeatureValues[actualDimension])
							m_maxFeatureValues[actualDimension] = temp;

						m_meanFeatureValues[actualDimension] += temp;

						actualDimension++;
					}
				}
				getline(myfile,line);
				m_dataPtsWithClass[i]->m_classification = atoi(line.data());
				i++;
			}
			//but if it isn't, just get to the next line without incrementing i.
			//this way every correct line will be inserted in sequence
			//without any gaps due to perhaps multiple line failures, etc.
			else
			{
				getline(myfile,line);
				badLines++;
			}
		}
		m_nPts = i;

		for(int j = 0; j < DIM; j++)
			m_meanFeatureValues[j] /= m_nPts;
	}
	else
	{
		LOG(ERROR, (format("File %1% at line %2%: Unable to open the training data file at %3%")%__LINE__%__FILE__%Config::Inst()->getPathTrainingFile()).str());
	}
	myfile.close();

	//Normalize the data points

	//Foreach feature within the data point
	for(uint j = 0;j < Config::Inst()->getEnabledFeatureCount();j++)
	{
		//Foreach data point
		for(int i=0;i < m_nPts;i++)
		{
			m_normalizedDataPts[i][j] = Normalize(m_normalization[j], m_dataPts[i][j], m_minFeatureValues[j], m_maxFeatureValues[j]);
		}
	}

	m_kdTree = new ANNkd_tree(					// build search structure
			m_normalizedDataPts,					// the data points
					m_nPts,						// number of points
					Config::Inst()->getEnabledFeatureCount());						// dimension of space
}

double ClassificationEngine::Normalize(normalizationType type, double value, double min, double max)
{
	switch (type)
	{
		case LINEAR:
		{
			return value / max;
		}
		case LINEAR_SHIFT:
		{
			return (value -min) / (max - min);
		}
		case LOGARITHMIC:
		{
			if(!value || !max)
				return 0;
			else return(log(value)/log(max));
			//return (log(value - min + 1)) / (log(max - min + 1));
		}
		case NONORM:
		{
			return value;
		}
		default:
		{
			//logger->Logging(ERROR, "Normalization failed: Normalization type unkown");
			return 0;
		}

		// TODO: A sigmoid normalization function could be very useful,
		// especially if we could somehow use it interactively to set the center and smoothing
		// while looking at the data visualizations to see what works best for a feature
	}
}


void ClassificationEngine::WriteDataPointsToFile(string outFilePath, ANNkd_tree* tree)
{
	ofstream myfile (outFilePath.data());

	if (myfile.is_open())
	{
		for(int i = 0; i < tree->nPoints(); i++ )
		{
			for(int j=0; j < tree->theDim(); j++)
			{
				myfile << tree->thePoints()[i][j] << " ";
			}
			myfile << m_dataPtsWithClass[i]->m_classification;
			myfile << "\n";
		}
	}
	else
	{
		LOG(ERROR, (format("File %1% at line %2%: Unable to open the training data file at %3%")%__LINE__%__FILE__%outFilePath).str());

	}
	myfile.close();

}
