//============================================================================
// Name        : KnnClassification.h
// Copyright   : DataSoft Corporation 2011-2013
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

#include "KnnClassification.h"
#include "Config.h"
#include "Lock.h"
#include "Suspect.h"

#include <sstream>

using namespace std;
using namespace Nova;

// Normalization method to use on each feature
// TODO: Make this a configuration var somewhere in Novaconfig.txt?
normalizationType KnnClassification::m_normalization[] =
{
		LINEAR_SHIFT, // Don't normalize IP traffic distribution, already between 0 and 1
		LINEAR_SHIFT,
		LOGARITHMIC,
		LOGARITHMIC,
		LOGARITHMIC,
		LOGARITHMIC,
		LOGARITHMIC,
		LOGARITHMIC,
		LOGARITHMIC,
		LINEAR_SHIFT,
		LINEAR_SHIFT,
		LINEAR_SHIFT,
		LINEAR_SHIFT,
		LINEAR_SHIFT
};

KnnClassification::KnnClassification()
{
	pthread_rwlock_init(&m_lock, NULL);

	m_normalizedDataPts = NULL;
	m_dataPts = NULL;
	m_kdTree = NULL;
}

KnnClassification::~KnnClassification()
{
	for(uint i = 0; i < m_dataPtsWithClass.size(); i++)
	{
		delete m_dataPtsWithClass[i];
	}
	if(m_kdTree != NULL)
	{
		delete m_kdTree;
	}
	if(m_dataPts != NULL)
	{
		annDeallocPts(m_dataPts);
	}
	if(m_normalizedDataPts != NULL)
	{
		annDeallocPts(m_normalizedDataPts);
	}
}

double KnnClassification::Classify(Suspect *suspect)
{
	Lock lock(&m_lock, READ_LOCK);
	double sqrtDIM = Config::Inst()->GetSqurtEnabledFeatures();
	int k = Config::Inst()->GetK();
	double d;
	FeatureIndex fi;

	FeatureSet fs = suspect->m_features;
	uint ai = 0;

	// Do we not have enough data to classify?
	if(fs.m_packetCount < Config::Inst()->GetMinPacketThreshold())
	{
		suspect->SetIsHostile(false);
		suspect->SetClassification(-2);
		return -2;
	}

	//Allocate the ANNpoint;
	ANNpoint aNN = annAllocPt(Config::Inst()->GetEnabledFeatureCount());
	if(aNN == NULL)
	{
		LOG(ERROR, "Classification engine has encountered an error.",
			"Classify had trouble allocating the ANN point. Aborting.");
		return -1;
	}

	ANNidxArray nnIdx = new ANNidx[k];			// allocate near neigh indices
	ANNdistArray dists = new ANNdist[k];		// allocate near neighbor dists

	vector<double> weights = Config::Inst()->GetFeatureWeights();
	//Iterate over the features, asserting the range is [min,max] and normalizing over that range
	for(int i = 0;i < DIM;i++)
	{
		if(Config::Inst()->IsFeatureEnabled(i))
		{
			if(fs.m_features[i] > m_maxFeatureValues[ai])
			{
				fs.m_features[i] = m_maxFeatureValues[ai];
			}
			else if(fs.m_features[i] < m_minFeatureValues[ai])
			{
				fs.m_features[i] = m_minFeatureValues[ai];
			}
			if(m_maxFeatureValues[ai] != 0)
			{
				aNN[ai] = Normalize(m_normalization[i], fs.m_features[i],
					m_minFeatureValues[ai], m_maxFeatureValues[ai], weights[i]);
			}
			else
			{
				LOG(ERROR, "Classification engine has encountered an error.",
					"Max value for a feature is 0. Normalization failed.");
			}
			ai++;
		}
	}

	m_kdTree->annkSearch(							// search
			aNN,								// query point
			k,									// number of near neighbors
			nnIdx,								// nearest neighbors (returned)
			dists,								// distance (returned)
			Config::Inst()->GetEps());								// error bound

	stringstream classificationNotes;

	for (int i = 0; i < k; i++)
	{
		classificationNotes << "k=" << i << ":d=" << dists[i];
		classificationNotes << ":c=" << m_dataPtsWithClass[nnIdx[i]]->m_classification;
		classificationNotes << ":i=" << nnIdx[i];
		classificationNotes << "\n:o ";
		for (uint j = 0; j < Config::Inst()->GetEnabledFeatureCount(); j++)
		{
			classificationNotes << m_dataPtsWithClass[nnIdx[i]]->m_annPoint[j] << " ";
		}

		classificationNotes << "\n:n ";

		for (uint j = 0; j < Config::Inst()->GetEnabledFeatureCount(); j++)
		{
			classificationNotes << m_kdTree->thePoints()[nnIdx[i]][j] << " ";
		}

		classificationNotes << endl << endl;;
	}
	suspect->m_classificationNotes = classificationNotes.str();

	for(int i = 0; i < DIM; i++)
	{
		fi = (FeatureIndex)i;
		suspect->SetFeatureAccuracy(fi, 0);
	}
	suspect->SetHostileNeighbors(0);

	//Determine classification according to weight by distance
	//	.5 + E[(1-Dist)*Class] / 2k (Where Class is -1 or 1)
	//	This will make the classification range from 0 to 1
	double classifyCount = 0;

	for(int i = 0; i < k; i++)
	{
		dists[i] = sqrt(dists[i]);				// unsquare distance
		for(int j = 0; j < DIM; j++)
		{
			if(Config::Inst()->IsFeatureEnabled(j))
			{
				double distance = (aNN[j] - m_kdTree->thePoints()[nnIdx[i]][j]);

				if(distance < 0)
				{
					distance *= -1;
				}

				fi = (FeatureIndex)j;
				d  = suspect->GetFeatureAccuracy(fi) + distance;
				suspect->SetFeatureAccuracy(fi, d);
			}
		}

		if(nnIdx[i] == -1)
		{
			stringstream ss;
			ss << "Unable to find a nearest neighbor for Data point " << i <<" Try decreasing the Error bound";
			LOG(ERROR, "Classification engine has encountered an error.", ss.str());
		}
		else
		{
			//If Hostile
			if(m_dataPtsWithClass[nnIdx[i]]->m_classification == 1)
			{
				classifyCount += (sqrtDIM - dists[i]);
				suspect->SetHostileNeighbors(suspect->GetHostileNeighbors()+1);
			}
			//If benign
			else if(m_dataPtsWithClass[nnIdx[i]]->m_classification == 0)
			{
				classifyCount -= (sqrtDIM - dists[i]);
			}
			else
			{
				stringstream ss;
				ss << "Data point has invalid classification. Should by 0 or 1, but is " << m_dataPtsWithClass[nnIdx[i]]->m_classification;
				LOG(ERROR, "Classification engine has encountered an error.", ss.str());
				suspect->SetClassification(-1);
				delete [] nnIdx;
			    delete [] dists;
			    annClose();
			    annDeallocPt(aNN);
				return -1;
			}
		}
	}
	for(int j = 0; j < DIM; j++)
	{
		fi = (FeatureIndex)j;
		d = suspect->GetFeatureAccuracy(fi) / k;
		suspect->SetFeatureAccuracy(fi, d);
	}

	suspect->SetClassification(.5 + (classifyCount / ((2.0*(double)k)*sqrtDIM )));

	// Fix for rounding errors caused by double's not being precise enough if DIM is something like 2
	if(suspect->GetClassification() < 0)
	{
		suspect->SetClassification(0);
	}
	else if(suspect->GetClassification() > 1)
	{
		suspect->SetClassification(1);
	}

	if(suspect->GetClassification() > Config::Inst()->GetClassificationThreshold())
	{
		suspect->SetIsHostile(true);
	}
	else
	{
		suspect->SetIsHostile(false);
	}

	delete [] nnIdx;
    delete [] dists;
    annClose();
    annDeallocPt(aNN);

	return suspect->GetClassification();
}

void KnnClassification::LoadConfiguration()
{
	this->LoadDataPointsFromFile(Config::Inst()->GetPathTrainingFile());
}

void KnnClassification::LoadDataPointsFromFile(string inFilePath)
{
	Lock lock(&m_lock, WRITE_LOCK);
	ifstream myfile (inFilePath.data());
	string line;
	vector<double> weights = Config::Inst()->GetFeatureWeights();

	// Clear max and min values
	for(int i = 0; i < DIM; i++)
	{
		m_maxFeatureValues[i] = 0;
	}

	for(int i = 0; i < DIM; i++)
	{
		m_minFeatureValues[i] = 0;
	}

	for(int i = 0; i < DIM; i++)
	{
		m_meanFeatureValues[i] = 0;
	}

	// Reload the data file
	if(m_dataPts != NULL)
	{
		annDeallocPts(m_dataPts);
	}
	if(m_normalizedDataPts != NULL)
	{
		annDeallocPts(m_normalizedDataPts);
	}

	//Delete any contents of the points list first
	for(uint i = 0; i < m_dataPtsWithClass.size(); i++)
	{
		delete m_dataPtsWithClass[i];
	}
	m_dataPtsWithClass.clear();

	//string array to check whether a line in data.txt file has the right number of fields
	string fieldsCheck[DIM];
	bool valid = true;

	int i = 0;
	int k = 0;
	int badLines = 0;

	//Count the number of data points for allocation
	if(myfile.is_open())
	{
		while(!myfile.eof())
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
		LOG(ERROR,"Classification Engine has encountered a problem",
			"Unable to open the training data file at "+ Config::Inst()->GetPathTrainingFile()+".");
	}

	myfile.close();
	int maxPts = i;

	//Open the file again, allocate the number of points and assign
	myfile.open(inFilePath.data(), ifstream::in);
	m_dataPts = annAllocPts(maxPts, Config::Inst()->GetEnabledFeatureCount()); // allocate data points
	m_normalizedDataPts = annAllocPts(maxPts, Config::Inst()->GetEnabledFeatureCount());

	if(myfile.is_open())
	{
		i = 0;

		while(!myfile.eof() && (i < maxPts))
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
				m_dataPtsWithClass.push_back(new Point(Config::Inst()->GetEnabledFeatureCount()));

				// Used for matching the 0->DIM index with the 0->Config::Inst()->getEnabledFeatureCount() index
				int actualDimension = 0;
				for(int defaultDimension = 0;defaultDimension < DIM;defaultDimension++)
				{
					double temp = strtod(fieldsCheck[defaultDimension].data(), NULL);

					if(Config::Inst()->IsFeatureEnabled(defaultDimension))
					{
						m_dataPtsWithClass[i]->m_annPoint[actualDimension] = temp;
						m_dataPts[i][actualDimension] = temp;

						//Set the max values of each feature. (Used later in normalization)
						if(temp > m_maxFeatureValues[actualDimension])
						{
							m_maxFeatureValues[actualDimension] = temp;
						}
						if(temp < m_minFeatureValues[actualDimension])
						{
							m_minFeatureValues[actualDimension] = temp;
						}

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

		stringstream ss;
		ss << "Loaded " << m_nPts << " data points into KNN tree" << endl;
		LOG(DEBUG, ss.str(), "");

		for(int j = 0; j < DIM; j++)
			m_meanFeatureValues[j] /= m_nPts;
	}
	else
	{
		LOG(ERROR,"Classification Engine has encountered a problem",
			"Unable to open the training data file at "+inFilePath+".");
	}
	myfile.close();

	//Normalize the data points
	uint ai = 0;
	for (uint i = 0; i < DIM; i++)
	{
		if (Config::Inst()->IsFeatureEnabled(i))
		{
			//Foreach data point
			for(int point=0; point < m_nPts; point++)
			{
				m_normalizedDataPts[point][ai] = Normalize(m_normalization[ai], m_dataPts[point][ai], m_minFeatureValues[ai], m_maxFeatureValues[ai], weights[i]);
			}

			ai++;
		}
	}

	if(m_kdTree != NULL)
	{
		delete m_kdTree;
	}
	m_kdTree = new ANNkd_tree(					// build search structure
			m_normalizedDataPts,					// the data points
					m_nPts,						// number of points
					Config::Inst()->GetEnabledFeatureCount());						// dimension of space
}

void KnnClassification::LoadDataPointsFromVector(vector<double*> points)
{
	Lock lock(&m_lock, WRITE_LOCK);
	vector<double> weights = Config::Inst()->GetFeatureWeights();
	// Clear max and min values
	for(int i = 0; i < DIM; i++)
	{
		m_maxFeatureValues[i] = 0;
	}

	for(int i = 0; i < DIM; i++)
	{
		m_minFeatureValues[i] = 0;
	}

	for(int i = 0; i < DIM; i++)
	{
		m_meanFeatureValues[i] = 0;
	}

	// Reload the data file
	if(m_dataPts != NULL)
	{
		annDeallocPts(m_dataPts);
	}
	if(m_normalizedDataPts != NULL)
	{
		annDeallocPts(m_normalizedDataPts);
	}

	//Delete any contents of the points list first
	for(uint i = 0; i < m_dataPtsWithClass.size(); i++)
	{
		delete m_dataPtsWithClass[i];
	}
	m_dataPtsWithClass.clear();

	//Open the file again, allocate the number of points and assign
	m_dataPts = annAllocPts(points.size(), Config::Inst()->GetEnabledFeatureCount()); // allocate data points
	m_normalizedDataPts = annAllocPts(points.size(), Config::Inst()->GetEnabledFeatureCount());


	for(uint i = 0; i < points.size(); i++)
	{
		m_dataPtsWithClass.push_back(new Point(Config::Inst()->GetEnabledFeatureCount()));

		// Used for matching the 0->DIM index with the 0->Config::Inst()->getEnabledFeatureCount() index
		int actualDimension = 0;
		for(int defaultDimension = 0;defaultDimension < DIM;defaultDimension++)
		{
			double temp = points.at(i)[defaultDimension];

			if(Config::Inst()->IsFeatureEnabled(defaultDimension))
			{
				m_dataPtsWithClass[i]->m_annPoint[actualDimension] = temp;
				m_dataPts[i][actualDimension] = temp;

				//Set the max values of each feature. (Used later in normalization)
				if(temp > m_maxFeatureValues[actualDimension])
				{
					m_maxFeatureValues[actualDimension] = temp;
				}
				if(temp < m_minFeatureValues[actualDimension])
				{
					m_minFeatureValues[actualDimension] = temp;
				}

				m_meanFeatureValues[actualDimension] += temp;

				actualDimension++;
			}
		}

		m_dataPtsWithClass[i]->m_classification = points.at(i)[DIM];
	}

	m_nPts = points.size();

	for(int j = 0; j < DIM; j++)
		m_meanFeatureValues[j] /= m_nPts;


	//Normalize the data points
	//Normalize the data points
	uint ai = 0;
	for (uint i = 0; i < DIM; i++)
	{
		if (Config::Inst()->IsFeatureEnabled(i))
		{
			//Foreach data point
			for(int point=0; point < m_nPts; point++)
			{
				m_normalizedDataPts[point][ai] = Normalize(m_normalization[ai], m_dataPts[point][ai], m_minFeatureValues[ai], m_maxFeatureValues[ai], weights[i]);
			}

			ai++;
		}
	}

	if(m_kdTree != NULL)
	{
		delete m_kdTree;
	}
	m_kdTree = new ANNkd_tree(					// build search structure
			m_normalizedDataPts,					// the data points
					m_nPts,						// number of points
					Config::Inst()->GetEnabledFeatureCount());						// dimension of space
}

double KnnClassification::Normalize(normalizationType type, double value, double min, double max, double weight)
{
	double ret = -1;
	switch(type)
	{
		case LINEAR:
		{
			ret = (value / max);
			break;
		}
		case LINEAR_SHIFT:
		{
			ret = ((value -min) / (max - min));
			break;
		}
		case NONORM:
		{
			ret = value;
			break;
		}
		case LOGARITHMIC:
		{
			ret = 0;
			//If neither are 0
			if(value && max)
			{
				ret = (log(value + 1)/log(max + 1));
			}
			break;
		}
		default:
		{
			LOG(ERROR, "Unknown normalization type", "");
			break;
		}
		// TODO: A sigmoid normalization function could be very useful,
		// especially if we could somehow use it interactively to set the center and smoothing
		// while looking at the data visualizations to see what works best for a feature
	}

	ret = ret*weight;
	if (ret < 0) {
		LOG(WARNING, "Normalize returned a negative number. This is probably an error", "");
	}
	return ret;
}
