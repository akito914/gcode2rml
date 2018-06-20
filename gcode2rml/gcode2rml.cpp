// gcode2rml.cpp
//

#define _CRT_USE_WINAPI_FAMILY_DESKTOP_APP
#define _USE_MATH_DEFINES


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "vector.h"
#include <sys\stat.h>


// variables

int absInc = 90;    // 90: abs      / 91: inc
int mmIn = 20;      // 20: inches   / 21: millimeters (mm)
int movMode = 0;    //  0: rapidPos /  1: linItp   /  2: CWCirItp / 3: CCWCirItp / 28: ReturnHome
int planeSlct = 17; // 17: XY plane / 18: XZ plane / 19: YZ plane
int TLOCMode = 49;  // 43: positive / 44: negative / 49: canceled
int TROCMode = 40;  // 40: canceled / 41: left     / 42: right
int feedMode = 94;  // 94: per min  / 95: per rev
int coorSys = 1;
VECTOR nextPos = { 0, 0, 0 };
VECTOR centorPosInc = { 0, 0, 0 };
double feedSpeed = 0;
double spindleSpeed = 0;
int spindleState = 0; // 0: stop / 1: CW / -1: CCW
int programNumber = 0;
int nextToolNum = 0;
int TROCToolNum = 0;
int TLOCToolNum = 0;
int dwellEnable = 0; // 0: disable     / 1: enable
int coorChanged = 0; // 0: not changed / 1: changed

// imported setting
VECTOR homePosition = VGet( 0.0, 0.0, 0.0 );	// home position		// setting number 0
VECTOR posOffset = VGet( 0.0, 0.0, 0.0 );		// position offset		// setting number 1
double rapidFeedSpeed = 1000;					// rapid feed speed		// setting number 2
double circularResolution = 360;				// circular resolution	// setting number 3

FILE *pSettingFile;
FILE *pInputFile;
FILE *pOutputFile;
VECTOR currentPos = { 0, 0, 0 };

long inputFileSize = 0;


// function prototypes

void importSetting(void);
double importValueSetting(char *str);
VECTOR importVectorSetting(char *str);


void loadWord(void);
void wordProcess(char *word);

void rapidPositioning(VECTOR nextPos);
void linearInterpolation(VECTOR nextPos, double feedSpeed);
void circularInterpolation(VECTOR nextPos, VECTOR centorPosInc, int planeSlct, int dir/* 2:CW 3:CCW */, double feedSpeed);
void circularInterpolationR(VECTOR nextPos, double R, int planeSlct, int dir/* 2:CW 3:CCW */, double feedSpeed);
void returnHome(VECTOR viaPos);
VECTOR planeConv(VECTOR vect, int plane);
VECTOR planeConvInv(VECTOR vect, int plane);

void move(VECTOR nextPos, double feedSpeed);

char* strRemove(char *str, char c);
double dmin3(double val1, double val2, double val3);
double dmax3(double val1, double val2, double val3);
int rangeChr(char chr, char min, char max);
long getFileSize(char *file);
void printErrorNum(errno_t errorNum);


int main(void)
{
	char inputFileName[260];
	char outputFileName[260];
	errno_t errorNum = 0;

	// open setting file
	errorNum = fopen_s(&pSettingFile, "setting.txt", "r");
	if (errorNum != 0)
	{
		printf("failed to open \"setting.txt\"\n");
		printErrorNum(errorNum);
		system("pause");
		return -1;
	}

	// open input file ( G code )
	printf("input file name : ");
	scanf_s("%s", inputFileName, 260);
	errorNum = fopen_s(&pInputFile, inputFileName, "rb");
	if (errorNum != 0)
	{
		printf("failed to open\n");
		printErrorNum(errorNum);
		fclose(pSettingFile);
		system("pause");
		return -1;
	}

	// open output file ( RML-1 )
	printf("output file name : ");
	scanf_s("%s", outputFileName, 260);
	errorNum = fopen_s(&pOutputFile, outputFileName, "w");
	if (errorNum != 0)
	{
		printf("failed to open\n");
		printErrorNum(errorNum);
		fclose(pSettingFile);
		fclose(pInputFile);
		system("pause");
		return -1;
	}

	printf("\n");


	inputFileSize = getFileSize(inputFileName);

	importSetting();

	fprintf(pOutputFile, ";;^IN;");
	fprintf(pOutputFile, "V85.0;");
	fprintf(pOutputFile, "^PR;");
	fprintf(pOutputFile, "Z0,0,15500;");
	fprintf(pOutputFile, "^PA;");

	loadWord();

	fprintf(pOutputFile, "^IN;");


	printf("\nCode conversion completed ! \n\n");

	fclose(pSettingFile);
	fclose(pInputFile);
	fclose(pOutputFile);
	system("pause");
    return 0;
}

void importSetting(void)
{
	char *valStr;
	char str[100];
	int ival;
	double dval;
	VECTOR vect;

	while (fgets(str, 100, pSettingFile) == str)
	{
		strtok_s(str, "=", &valStr);

		strRemove(str, ' ');
		strRemove(str, '\r');
		strRemove(str, '\n');
		strRemove(valStr, ' ');
		strRemove(valStr, '\r');
		strRemove(valStr, '\n');
		strRemove(valStr, '(');
		strRemove(valStr, ')');

		if (str[0] != '\0' && str[0] != '#')
		{

			if (strcmp(str, "homePosition") == 0)
			{
				if(sscanf_s(valStr, "%lf,%lf,%lf", &(vect.x), &(vect.y), &(vect.z)) == 3)
					homePosition = vect;
			}
			else if (strcmp(str, "posOffset") == 0)
			{
				if(sscanf_s(valStr, "%lf,%lf,%lf", &(vect.x), &(vect.y), &(vect.z)) == 3)
					posOffset = vect;

			}
			else if (strcmp(str, "rapidFeedSpeed") == 0)
			{
				if(sscanf_s(valStr, "%lf", &dval) == 1)
					rapidFeedSpeed = dval;
			}
			else if (strcmp(str, "circularResolution") == 0)
			{
				if(sscanf_s(valStr, "%lf", &dval) == 1)
					circularResolution = dval;
			}

		}
	}

}

double importValueSetting(char *str)
{
	return 0;
}

VECTOR importVectorSetting(char *str)
{
	return VGet(0, 0, 0);
}


void loadWord(void)
{
	static char c, pc = 0;
	static char word[100] = "";
	static int cCount = 0;
	static int comment = 0;
	static long byteCount = 0;
	static long p_byteCount = 0;

	while (true)
	{

		if (byteCount - p_byteCount >= inputFileSize * 0.01)
		{
			printf("\rprogress : %.1f%%", byteCount / (double)inputFileSize * 100.0);
			p_byteCount = byteCount;
		}
		byteCount++;

		if (fread(&c, sizeof(char), 1, pInputFile) != 1)
		{
			if (rangeChr(word[0], 'A', 'Z') == 0)
			{
				word[cCount] = '\0';
				cCount = 0;
				wordProcess(word);
			}
			printf("\rprogress : %.1f%%\n", (byteCount - 1) / (double)inputFileSize * 100.0);
			return;
		}

		switch (c)
		{
		case '%':
			break;

		case '(':
			comment = 1;
			break;

		case ')':
			comment = 0;
			break;

		case ' ': case '\r': case '\n': case ';':
			break;

		default:
			if (comment == 1)break;

			if (rangeChr(c, 'A', 'Z') == 0 && rangeChr(word[0], 'A', 'Z') == 0)
			{
				// アドレスを受け取ると前回ワードの処理を行う
				word[cCount] = '\0';
				cCount = 0;
				wordProcess(word);
				if (pc == '\n' || pc == ';')wordProcess(";0"); // ワードの実行
			}

			word[cCount] = c;
			cCount += 1;
			break;
		}
		pc = c;

	}

}


void wordProcess(char *word)
{
	double fval;
	int ival;
	char address;
	char valStr[20];
	double dwellTime = 0;

	address = word[0];
	strcpy_s(valStr, word + 1);
	
	switch (address)
	{
	case ';': // end of block
		if (coorChanged == 0)break;

		switch (movMode) {
		case 0:
			rapidPositioning(nextPos);
			break;

		case 1:
			linearInterpolation(nextPos, feedSpeed);
			break;

		case 2:
			circularInterpolation(nextPos, centorPosInc, planeSlct, movMode, feedSpeed);
			break;

		case 3:
			circularInterpolation(nextPos, centorPosInc, planeSlct, movMode, feedSpeed);
			break;

		case 28:
			returnHome(nextPos);
			break;

		default:
			break;

		}
		coorChanged = 0;
		centorPosInc = VGet(0, 0, 0);
		break;

	case 'D': // Tool radius compensation tool number
		if (TROCMode == 40)break;
		sscanf_s(valStr, "%d", &ival);
		TROCToolNum = ival;
		if (ival == 0)TROCMode = 40;
		break;

	case 'F':
		sscanf_s(valStr, "%lf", &fval);
		feedSpeed = fval;
		break;

	case 'G':
		sscanf_s(valStr, "%d", &ival);
		switch (ival) {
		case 0: // Rapid positioning
			movMode = 0;
			break;

		case 1: // Linear interpolation
			movMode = 1;
			break;

		case 2: // Circular interpolation, clockwise
			movMode = 2;
			break;

		case 3: // Circular interpolation, counterclockwise
			movMode = 3;
			break;

		case 4: // Dwell
			dwellEnable = 1;
			break;

		case 17: // XY plane selection
			planeSlct = 17;
			break;

		case 18: // XZ plane selection
			planeSlct = 18;
			break;

		case 19: // YZ plane selection
			planeSlct = 19;
			break;

		case 20: // Programming in inches
			mmIn = 20;
			break;

		case 21: // Programming in millimeters (mm)
			mmIn = 21;
			break;

		case 28: // Return to home position
			movMode = 28;
			break;

		case 40: // Tool radius compensation off
			TROCMode = 40;
			TROCToolNum = 0;
			break;

		case 41: // Tool radius compensation left
			TROCMode = 41;
			break;

		case 42: // Tool radius compensation right
			TROCMode = 42;
			break;

		case 43: // Tool height offset compensation positive
			TLOCMode = 43;
			break;

		case 44: // Tool height offset compensation negative
			TLOCMode = 44;
			break;

		case 49: // Tool length offset compensation cancel
			TLOCMode = 49;
			break;

		case 54: case 55: case 56: case 57: case 58: case 59:// Work coordinate systems
			coorSys = ival - 54;
			break;

		case 80: // Cancel canned cycle
			break;

		case 90: // Absolute programming
			absInc = 90;
			fprintf(pOutputFile, "^PA;");
			break;

		case 91: // Incremental programming
			absInc = 91;
			fprintf(pOutputFile, "^PR;");
			break;

		case 94: // Feed Per Minute
			feedMode = 94;
			break;

		case 95: // Feed Per Rev
			feedMode = 95;
			break;

		default:
			// unknown code
			break;
		}
		break;

	case 'H': // Tool length offset compensation tool number
		if (TLOCMode == 49)break;
		sscanf_s(valStr, "%d", &ival);
		TLOCToolNum = ival;
		if (ival == 0)TLOCMode = 49;
		break;

	case 'I':
		sscanf_s(valStr, "%lf", &fval);
		centorPosInc.x = fval;
		break;

	case 'J':
		sscanf_s(valStr, "%lf", &fval);
		centorPosInc.y = fval;
		break;

	case 'K':
		sscanf_s(valStr, "%lf", &fval);
		centorPosInc.z = fval;
		break;

	case 'M':
		sscanf_s(valStr, "%d", &ival);
		switch (ival) {
		case 0: // Program Stop
			break;

		case 1: // Optional Stop
			break;

		case 2: // Program end
			break;

		case 3: // Spindle CW
			spindleState = 1;
			fprintf(pOutputFile,"!RC15;!MC1;");
			break;

		case 4: // Spindle CCW
			spindleState = -1;
			fprintf(pOutputFile, "!RC15;!MC1;");
			break;

		case 5: // spindle Stop
			spindleState = 0;
			fprintf(pOutputFile, "!MC0;");
			break;

		case 6: // Tool Change
			break;

		case 8: // Coolant ON
			break;

		case 9: // Coolant OFF
			break;

		case 30: // End of Tape
			break;

		}
		break;

	case 'N': // block number
		break;

	case 'O': // program number
		sscanf_s(valStr, "%d", &ival);
		programNumber = ival;

		break;

	case 'P':
		if (dwellEnable == 0)break;
		dwellEnable = 0;
		if (strstr(valStr, ".") != NULL)
		{
			sscanf_s(valStr, "%lf", &fval);
			dwellTime = fval;
		}
		else
		{
			sscanf_s(valStr, "%d", &ival);
			dwellTime = ival / 1000.0;
		}
		break;

	case 'S':
		sscanf_s(valStr, "%lf", &fval);
		spindleSpeed = fval;
		break;

	case 'T': // next tool set
		sscanf_s(valStr, "%d", &ival);
		nextToolNum = ival;
		break;

	case 'X':
		sscanf_s(valStr, "%lf", &fval);
		if (absInc == 91)nextPos.x = nextPos.x + fval;
		else nextPos.x = fval;
		coorChanged = 1;
		break;

	case 'Y':
		sscanf_s(valStr, "%lf", &fval);
		if (absInc == 91)nextPos.y = nextPos.y + fval;
		else nextPos.y = fval;
		coorChanged = 1;
		break;

	case 'Z':
		sscanf_s(valStr, "%lf", &fval);
		if (absInc == 91)nextPos.z = nextPos.z + fval;
		else nextPos.z = fval;
		coorChanged = 1;
		break;

	default:
		// unknown code
		break;

	}

}


void rapidPositioning(VECTOR nextPos)
{
	move(nextPos, rapidFeedSpeed);
}

void linearInterpolation(VECTOR nextPos, double feedSpeed)
{
	move(nextPos, feedSpeed);
}

void circularInterpolation(VECTOR nextPos, VECTOR centorPosInc, int planeSlct, int dir/* 2:CW 3:CCW */, double feedSpeed)
{
	VECTOR midStartPos, midCurrentPos, midNextPos, midCentorPosInc;
	VECTOR midCentorPos, midDelta1, midDelta2;
	VECTOR pos = VGet(0.0, 0.0, 0.0);
	double temp, deltaAngle;
	int deltaSteps;

	midCentorPos = VGet(0, 0, 0);

	midCurrentPos = planeConv(currentPos, planeSlct);
	midNextPos = planeConv(nextPos, planeSlct);
	midCentorPosInc = planeConv(centorPosInc, planeSlct);
	midCentorPos = VAdd(currentPos, midCentorPosInc);

	midStartPos = midCurrentPos;
	midDelta1 = VSub(midCurrentPos, midCentorPos);
	midDelta2 = VSub(midNextPos, midCentorPos);

	midDelta1.z = 0.0;
	midDelta2.z = 0.0;
	deltaAngle = acos(VDot(midDelta1, midDelta2) / (VSize(midDelta2) * VSize(midDelta1)));
	if (deltaAngle <= 0)deltaAngle += 2 * M_PI;
	deltaSteps = circularResolution * deltaAngle / (2 * M_PI);

	for (int step = 0; step < deltaSteps; step++) {
		double thetaTemp = 2 * M_PI * step / circularResolution;
		if (dir == 2)thetaTemp *= -1;
		pos.x = midDelta1.x * cos(thetaTemp) + midDelta1.y * sin(-thetaTemp) + midCentorPos.x;
		pos.y = midDelta1.x * sin(thetaTemp) + midDelta1.y * cos(thetaTemp) + midCentorPos.y;
		pos.z = (midNextPos.z - midStartPos.z) * (thetaTemp / deltaAngle) + midStartPos.z;
		move(planeConvInv(pos, planeSlct), feedSpeed);
		currentPos = planeConvInv(pos, planeSlct);
	}
	move(nextPos, feedSpeed);
	currentPos = nextPos;
}

void circularInterpolationR(VECTOR nextPos, double R, int planeSlct, int dir/* 2:CW 3:CCW */, double feedSpeed)
{
	VECTOR midStartPos, midCurrentPos, midNextPos;
	VECTOR midCentorPos, midDelta1, midDelta2;
	VECTOR midMidPoint, midDelta, midNormUnit;
	VECTOR pos = VGet(0.0, 0.0, 0.0);
	double temp, deltaAngle;
	int deltaSteps;
	double oneDeg = M_PI / 180;

	midCentorPos = VGet(0, 0, 0);
	midCurrentPos = planeConv(currentPos, planeSlct);
	midNextPos = planeConv(nextPos, planeSlct);
	midStartPos = midCurrentPos;

	midMidPoint = VScale(VAdd(midStartPos, midNextPos), 0.5);
	midDelta = VSub(midNextPos, midStartPos);
	midMidPoint.z = 0.0;
	midDelta.z = 0.0;
	midNormUnit = VNorm(VCross(midDelta, VGet(0.0, 0.0, 1.0))); // 法単位ベクトルの計算
																
	// 中心点の算出
	if (fabs(R) <= VSize(midDelta) / 2.0)midCentorPos = midMidPoint;
	else midCentorPos = VAdd(midMidPoint, VScale(midNormUnit, ((R < 0) ? -1 : 1) * ((dir != 2) ? -1 : 1) * sqrt(R*R - VSize(midDelta) * VSize(midDelta) / 4.0)));

	midDelta1 = VSub(midStartPos, midCentorPos);
	midDelta2 = VSub(midNextPos, midCentorPos);

	midDelta1.z = 0.0;
	midDelta2.z = 0.0;
	deltaAngle = acos(VDot(midDelta1, midDelta2) / (VSize(midDelta2) * VSize(midDelta1)));
	if (deltaAngle <= 0) deltaAngle += 2 * M_PI;
	if (R < 0.0)deltaAngle = 2 * M_PI - deltaAngle;
	deltaSteps = circularResolution * deltaAngle / (2 * M_PI);

	for (int step = 0; step < deltaSteps; step++) {
		double thetaTemp = 2 * M_PI * step / circularResolution;
		if (dir == 2)thetaTemp *= -1;
		pos.x = midDelta1.x * cos(thetaTemp) + midDelta1.y * sin(-thetaTemp) + midCentorPos.x;
		pos.y = midDelta1.x * sin(thetaTemp) + midDelta1.y * cos(thetaTemp) + midCentorPos.y;
		pos.z = (midNextPos.z - midStartPos.z) * (thetaTemp / deltaAngle) + midStartPos.z;
		move(planeConvInv(pos, planeSlct), feedSpeed);
		currentPos = planeConvInv(pos, planeSlct);
	}
	move(nextPos, feedSpeed);
	currentPos = nextPos;
}

void returnHome(VECTOR viaPos)
{
	rapidPositioning(viaPos);
	rapidPositioning(homePosition);
}

void move(VECTOR nextPos, double feedSpeed)
{
	static double lastFeedSpeed = -1.0;
	VECTOR outputPos;
	if (feedSpeed != lastFeedSpeed)
	{
		fprintf(pOutputFile, "V%.1f;", feedSpeed / 60);
		lastFeedSpeed = feedSpeed;
	}
	outputPos = VAdd(nextPos, posOffset);
	outputPos = VScale(outputPos, 100.0);
	outputPos = VAdd(outputPos, VGet(0.5, 0.5, 0.5));
	fprintf(pOutputFile, "Z%d,%d,%d;", (int)outputPos.x, (int)outputPos.y, (int)outputPos.z);
	currentPos = nextPos;
	return;
}


// Convert coordinates so that the axis crossing the plane becomes the Z axis .
VECTOR planeConv(VECTOR vect, int plane)
{
	VECTOR temp;
	switch (plane)
	{
	case 17: // XY plane
		temp.x = vect.x; temp.y = vect.y; temp.z = vect.z; break;
	case 18: // ZX plane
		temp.x = vect.z; temp.y = vect.x; temp.z = vect.y; break;
	case 19: // YZ plane
		temp.x = vect.y; temp.y = vect.z; temp.z = vect.x; break;
	}
	return temp;
}

// Restore converted coordinates .
VECTOR planeConvInv(VECTOR vect, int plane)
{
	VECTOR temp;
	switch (plane)
	{
	case 17: // XY plane
		temp.x = vect.x; temp.y = vect.y; temp.z = vect.z; break;
	case 18: // ZX plane
		temp.z = vect.x; temp.x = vect.y; temp.y = vect.z; break;
	case 19: // YZ plane
		temp.y = vect.x; temp.z = vect.y; temp.x = vect.z; break;
	}
	return temp;
}


char* strRemove(char *str, char c)
{
	int cCount = 0;
	int posCount = 0;
	for (cCount = 0; str[cCount] != '\0'; cCount++)
	{
		if (str[cCount] != c)
		{
			str[posCount] = str[cCount];
			posCount++;
		}
	}
	str[posCount] = '\0';
	return str;
}

// Returns the smallest nonzero value among the three values .
double dmin3(double val1, double val2, double val3)
{
	double temp;
	if (val1 > val2) { temp = val1; val1 = val2; val2 = temp; }
	if (val2 > val3) { temp = val2; val2 = val3; val3 = temp; }
	if (val1 > val2) { temp = val1; val1 = val2; val2 = temp; }
	if (val1 != 0.0)return val1;
	if (val2 != 0.0)return val2;
	return val3;
}

// Returns the largest of the three values .
double dmax3(double val1, double val2, double val3)
{
	double temp;
	if (val1 < val2) { temp = val1; val1 = val2; val2 = temp; }
	if (val2 < val3) { temp = val2; val2 = val3; val3 = temp; }
	if (val1 < val2) { temp = val1; val1 = val2; val2 = temp; }
	return val1;
}

// Returns whether character code is in range of from min to max
int rangeChr(char chr, char min, char max)
{
	if (chr >= min && chr <= max)return 0;
	return -1;
}

long getFileSize(char *file)
{
	struct stat statBuf;

	if (stat(file, &statBuf) == 0)
		return statBuf.st_size;

	return -1L;
}

// Prints error message indicated by error number .
void printErrorNum(errno_t errorNum)
{
	char errmsg[50];
	strerror_s(errmsg, 50, errorNum);
	puts(errmsg);
}



