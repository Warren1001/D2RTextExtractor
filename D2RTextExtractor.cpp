using namespace std;

#define _SILENCE_EXPERIMENTAL_FILESYSTEM_DEPRECATION_WARNING

#include <iostream>
#include <Windows.h>
#include <limits.h>
#include <sstream>
#include <string>
#include <experimental/filesystem> // need to be on c++14, 17+ is incompatible with some of the libraries used (ambiguous byte definitions everywhere)
#include "CascLib.h"

void make_txt(const char* d2name, const char* txtname);

static bool createDirectoryRecursive(string const& dirName, error_code& err) {
	namespace fs = experimental::filesystem;
	err.clear();
	string properDir = dirName.substr(0, dirName.find_last_of('\\'));
	if (!fs::create_directories(properDir, err)) {
		if (fs::exists(properDir)) {
			err.clear();
			return true;
		}
		return false;
	}
	return true;
}

static string appendPath(string path1, string path2) {
	if (path1.empty()) {
		return path2;
	}
	else {
		ostringstream stream;
		stream << path1 << "\\" << path2;
		return stream.str();
	}
}

static DWORD extractFile(HANDLE casc, string filePath, string extractDirectory) {

	HANDLE cascFile = NULL;
	HANDLE diskFile = NULL;
	DWORD fileSize = 0;
	DWORD error = ERROR_SUCCESS;
	error_code code;

	string extractPath = appendPath(extractDirectory, filePath.substr(5));

	if (!CascOpenFile(casc, filePath.c_str(), 0, 0, &cascFile)) {
		error = GetLastError();
		cout << "Error " << error << " opening casc file at path: " << filePath << endl;
	}

	if (error == ERROR_SUCCESS) {
		fileSize = CascGetFileSize(cascFile, NULL);
		if (fileSize == CASC_INVALID_SIZE) {
			error = GetLastError();
			cout << "Error " << error << " getting size of casc file at path: " << filePath << endl;
		}
	}

	if (error == ERROR_SUCCESS) {
		char* buffer = new char[fileSize];
		DWORD ignore = 0;

		if (!CascReadFile(cascFile, buffer, fileSize, &ignore)) {
			error = GetLastError();
			cout << "Error " << error << " reading casc file at path: " << filePath << endl;
		}

		if (error == ERROR_SUCCESS) {
			if (!createDirectoryRecursive(extractPath, code)) {
				error = code.value();
				cout << "Error " << error << " creating directories for path '" << extractPath << "': " << code.message() << endl;
			}
		}

		if (error == ERROR_SUCCESS) {
			diskFile = CreateFile(extractPath.c_str(), GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, NULL);
			if (diskFile == INVALID_HANDLE_VALUE) {
				error = GetLastError();
				cout << "Error " << error << " creating disk file at path: " << extractPath << endl;
			}
		}

		if (error == ERROR_SUCCESS) {
			if (!WriteFile(diskFile, buffer, fileSize, &fileSize, NULL)) {
				error = GetLastError();
				cout << "Error " << error << " writing to disk file at path: " << extractPath << endl;
			}
			else {
				cout << "Successfully extracted file to path: " << extractPath << endl;
			}
		}

		delete[] buffer;
	}

	if (diskFile != NULL) {
		CloseHandle(diskFile);
	}
	if (cascFile != NULL) {
		CascCloseFile(cascFile);
	}

	return error;
}

int main(int argc, char* argv[]) {

	if (argc != 3) {
		cout << "Expected two command line arguments, found " << argc - 1 << endl;
		return -1;
	}

	string cascPath = string(argv[1]); // path to D2R folder
	string extractDirectory = string(argv[2]); // path to extract data files out to

	string stringsFileNames[] = {
		"item-gems", "item-modifiers", "item-nameaffixes", "item-names",
		"item-runes", "levels", "mercenaries", "monsters", "npcs",
		"objects", "quests", "shrines", "skills", "ui", "vo"
	};

	HANDLE casc = NULL;
	HANDLE search = NULL;
	CASC_FIND_DATA findData;
	DWORD error = ERROR_SUCCESS;

	if (!CascOpenStorage(cascPath.c_str(), 0, &casc)) {
		error = GetLastError();
		cout << "Error " << error << " opening storage at path: " << cascPath << endl;
		return -1;
	}

	cout << "Opened storage at path: " << cascPath << endl;

	// data\global\animdata.d2
	// data\global\dataversionbuild.txt
	// data\hd\global\excel\desecratedzones.json
	// data\global\excel\*.txt
	// data\local\lng\strings\*?.json - see stringsFileNames above

	error = extractFile(casc, "data:data\\global\\animdata.d2", extractDirectory);
	if (error == ERROR_SUCCESS) {
		string pathBinary = appendPath(extractDirectory, "data\\global\\animdata.d2");
		string pathTxt = appendPath(extractDirectory, "data\\global\\animdata.txt");
		make_txt(pathBinary.c_str(), pathTxt.c_str());
		DeleteFile(pathBinary.c_str());
	}

	extractFile(casc, "data:data\\global\\dataversionbuild.txt", extractDirectory);
	extractFile(casc, "data:data\\hd\\global\\excel\\desecratedzones.json", extractDirectory);

	search = CascFindFirstFile(casc, "data:data\\global\\excel\\*.txt", &findData, NULL);
	if (search == INVALID_HANDLE_VALUE) {
		error = GetLastError();
		cout << "Error " << error << " finding a casc file at the path: data\\global\\excel\\*.txt" << endl;
	}

	if (error == ERROR_SUCCESS) {

		while (true) {

			extractFile(casc, findData.szFileName, extractDirectory);

			if (!CascFindNextFile(search, &findData)) {
				break;
			}

		}
	}

	if (search != NULL) {
		CascFindClose(search);
	}

	for (string stringsFileName : stringsFileNames) {
		ostringstream ss;
		ss << "data:data\\local\\lng\\strings\\" << stringsFileName << ".json";
		extractFile(casc, ss.str(), extractDirectory);
	}

	if (casc != NULL) {
		CascCloseStorage(casc);
	}

	if (error == ERROR_SUCCESS) {
		return 0;
	}
	return -1;
}

/*
 *	the following not written by me (lightly tweaked). this is animdata_edit source, can be found on phrozenkeep
*/

#define _CRT_SECURE_NO_DEPRECATE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef unsigned char UBYTE;


// ----------------------------------------------------------------------
#pragma pack(1) // no padding bytes between members of a struct for now
// ----------------------------------------------------------------------
typedef struct
{
	char  cof_name[8];    // 7 chars + 1 zero termination (0x00)
	long  frames_per_dir; // frames per direction (0 ~ 256 normally)
	long  anim_speed;     // animation speed, in 256th of 25fps. 128=50% of 25fps = 12.5fps
	UBYTE frame_tag[144]; // tag value for each frame

	UBYTE hash;           // NOT present in AnimData.d2 !
} ANIMDATA_RECORD_S;
// ----------------------------------------------------------------------
#pragma pack()  // cancel the previous "no padding bytes" command
// ----------------------------------------------------------------------

typedef struct ANIMDATA_LINK_S
{
	ANIMDATA_RECORD_S* record;
	struct ANIMDATA_LINK_S* next;
} ANIMDATA_LINK_S;

ANIMDATA_LINK_S* animdata_hash[256];
ANIMDATA_LINK_S** animdata_sorted_name = NULL;
ANIMDATA_RECORD_S** animdata_records = NULL;
char              buffer[512];

// ==========================================================================
void animdata_edit_init(void)
{
	memset(animdata_hash, 0, sizeof(animdata_hash));
}


// ==========================================================================
void animdata_edit_atexit(void)
{
	ANIMDATA_LINK_S* link, * next;
	int i;


	for (i = 0; i < 256; i++)
	{
		link = animdata_hash[i];
		while (link != NULL)
		{
			next = link->next;
			free(link);
			link = next;
		}
	}

	if (animdata_sorted_name != NULL)
		free(animdata_sorted_name);

	if (animdata_records != NULL)
		free(animdata_records);
}


// ==========================================================================
void* load_file_in_mem(const char* name, long* ptr_size)
{
	FILE* in;
	void* buffer;


	in = fopen(name, "rb");
	if (in == NULL)
	{
		printf("can't open \"%s\"\n", name);
		return NULL;
	}
	fseek(in, 0, SEEK_END);
	*ptr_size = ftell(in);
	fseek(in, 0, SEEK_SET);

	// buffer
	buffer = (void*)malloc(*ptr_size);
	if (buffer == NULL)
	{
		fclose(in);
		printf("can't allocate %li bytes for copying \"%s\" in memory\n",
			*ptr_size,
			name
		);
		return NULL;
	}

	// read
	if (fread(buffer, *ptr_size, 1, in) != 1)
	{
		free(buffer);
		fclose(in);
		printf("Disk-error while reading \"%s\"\n", name);
		printf("Maybe the file is still open into another application ?\n");
		return NULL;
	}

	// end
	fclose(in);
	return buffer;
}


// ==========================================================================
void insert_record(int hash, ANIMDATA_RECORD_S* record)
{
	ANIMDATA_LINK_S* new_link;


	if ((hash < 0) || (hash > 255))
		return;

	new_link = (ANIMDATA_LINK_S*)malloc(sizeof(ANIMDATA_LINK_S));
	if (new_link == NULL)
	{
		printf("can't allocate %i bytes for adding record '%.7s' to the hash list # %i\n",
			(int)sizeof(ANIMDATA_LINK_S),
			record->cof_name,
			hash
		);
		return;
	}
	new_link->record = record;
	new_link->next = animdata_hash[hash];
	animdata_hash[hash] = new_link;
}


// ==========================================================================
int animdata_edit_cmp(const void* arg1, const void* arg2)
{
	ANIMDATA_LINK_S* link1 = (ANIMDATA_LINK_S*)*((ANIMDATA_LINK_S**)arg1),
		* link2 = (ANIMDATA_LINK_S*)*((ANIMDATA_LINK_S**)arg2);
	char* str1 = link1->record->cof_name,
		* str2 = link2->record->cof_name;


	return _stricmp(str1, str2);
}


// ==========================================================================
long animdata_sort_name(void)
{
	ANIMDATA_LINK_S* link;
	long            nb_rec, size, x;
	int             i;


	// count nb records
	nb_rec = 0;
	for (i = 0; i < 256; i++)
	{
		link = animdata_hash[i];
		while (link != NULL)
		{
			nb_rec++;
			link = link->next;
		}
	}

	// malloc
	size = sizeof(ANIMDATA_LINK_S*) * nb_rec;
	animdata_sorted_name = (ANIMDATA_LINK_S**)malloc(size);
	if (animdata_sorted_name == NULL)
	{
		printf("can't allocate %li bytes for animdata_sorted_name\n", size);
		return 0;
	}
	memset(animdata_sorted_name, 0, size);

	// prepare the sort
	x = 0;
	for (i = 0; i < 256; i++)
	{
		link = animdata_hash[i];
		while (link != NULL)
		{
			animdata_sorted_name[x] = link;
			x++;
			link = link->next;
		}
	}

	// sort them
	qsort(animdata_sorted_name, nb_rec, sizeof(ANIMDATA_LINK_S*), animdata_edit_cmp);

	// end
	return nb_rec;
}


// ==========================================================================
void make_txt(const char* d2name, const char* txtname)
{
	ANIMDATA_RECORD_S* record;
	ANIMDATA_LINK_S* link;
	UBYTE* buffer;
	long              size, * dw_ptr, nb_rec, cursor, r;
	int               i, x;
	FILE* out;


	buffer = (UBYTE*)load_file_in_mem(d2name, &size);
	if (buffer == NULL)
	{
		return;
	}

	if (size == 0)
	{
		printf("\"%s\" is empty\n", d2name);
		return;
	}

	// read animdata.d2 and fill animdata_hash[] lists
	cursor = 0;
	for (i = 0; i < 256; i++)
	{
		if ((cursor + 4) <= size) // nb_rec require 4 bytes
		{
			dw_ptr = (long*)(buffer + cursor);
			nb_rec = *dw_ptr;
			cursor += 4;
			for (r = 0; r < nb_rec; r++)
			{
				// read a record
				if ((cursor + 160) <= size) // a record is 160 bytes
				{
					record = (ANIMDATA_RECORD_S*)(buffer + cursor);
					insert_record(i, record);
					cursor += 160;
				}
			}
		}
	}

	// write txt datas
	out = fopen(txtname, "wt");
	if (out == NULL)
	{
		freopen("error.txt", "wt", stdout);
		printf("can't open %s\n", txtname);
		exit(1);
	}
	fputs("CofName"
		"\tFramesPerDirection"
		"\tAnimationSpeed"
		"\tFrameData000",
		out
	);
	for (i = 1; i <= 143; i++)
		fprintf(out, "\tFrameData%03i", i);
	fprintf(out, "\n");

	// sort cof names
	nb_rec = animdata_sort_name();

	// write them
	for (r = 0; r < nb_rec; r++)
	{
		link = animdata_sorted_name[r];
		fprintf(out, "%.7s", link->record->cof_name);
		fprintf(out, "\t%li", link->record->frames_per_dir);
		fprintf(out, "\t%li", link->record->anim_speed);
		for (x = 0; x < 144; x++)
			fprintf(out, "\t%i", link->record->frame_tag[x]);
		fprintf(out, "\n");
	}

	// end
	fclose(out);
	free(buffer);
	printf("%li records writen into %s\n", nb_rec, txtname);
}
