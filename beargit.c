#include <stdio.h>
#include <string.h>

#include <unistd.h>
#include <sys/stat.h>

#include "beargit.h"
#include "util.h"

/* Implementation Notes:
 *
 * - Functions return 0 if successful, 1 if there is an error.
 * - All error conditions in the function description need to be implemented
 *   and written to stderr. We catch some additional errors for you in main.c.
 * - Output to stdout needs to be exactly as specified in the function description.
 * - Only edit this file (beargit.c)
 * - Here are some of the helper functions from util.h:
 *   * fs_mkdir(dirname): create directory <dirname>
 *   * fs_rm(filename): delete file <filename>
 *   * fs_mv(src,dst): move file <src> to <dst>, overwriting <dst> if it exists
 *   * fs_cp(src,dst): copy file <src> to <dst>, overwriting <dst> if it exists
 *   * write_string_to_file(filename,str): write <str> to filename (overwriting contents)
 *   * read_string_from_file(filename,str,size): read a string of at most <size> (incl.
 *     NULL character) from file <filename> and store it into <str>. Note that <str>
 *     needs to be large enough to hold that string.
 *  - You NEED to test your code. The autograder we provide does not contain the
 *    full set of tests that we will run on your code. See "Step 5" in the project spec.
 */

/* beargit init
 *
 * - Create .beargit directory
 * - Create empty .beargit/.index file
 * - Create .beargit/.prev file containing 0..0 commit id
 *
 * Output (to stdout):
 * - None if successful
 */

int beargit_init(void) 
{
  fs_mkdir(".beargit");

  FILE* findex = fopen(".beargit/.index", "w");
  fclose(findex);

  FILE* fbranches = fopen(".beargit/.branches", "w");
  fprintf(fbranches, "%s\n", "master");
  fclose(fbranches);

  write_string_to_file(".beargit/.prev", "0000000000000000000000000000000000000000");
  write_string_to_file(".beargit/.current_branch", "master");

  fopen(".beargit/.branch_master", "w");
  fs_cp(".beargit/.prev", ".beargit/.branch_master");

  return 0;
}



/* beargit add <filename>
 *
 * - Append filename to list in .beargit/.index if it isn't in there yet
 *
 * Possible errors (to stderr):
 * >> ERROR:  File <filename> has already been added.
 *
 * Output (to stdout):
 * - None if successful
 */

int beargit_add(const char* filename) 
{
  FILE* findex = fopen(".beargit/.index", "r");
  FILE *fnewindex = fopen(".beargit/.newindex", "w");

  char line[FILENAME_SIZE];
  while(fgets(line, sizeof(line), findex)) 
  {
    strtok(line, "\n");
    if (strcmp(line, filename) == 0) 
    {
      fprintf(stderr, "ERROR:  File %s has already been added.\n", filename);
      fclose(findex);
      fclose(fnewindex);
      fs_rm(".beargit/.newindex");
      return 3;
    }

    fprintf(fnewindex, "%s\n", line);
  }

  fprintf(fnewindex, "%s\n", filename);
  fclose(findex);
  fclose(fnewindex);

  fs_mv(".beargit/.newindex", ".beargit/.index");

  return 0;
}

/* beargit status
 *
 * See "Step 1" in the project spec.
 *
 */

int beargit_status() 
{
  FILE* findex = fopen(".beargit/.index", "r");

  printf("Tracked files:\n\n");

  char line[FILENAME_SIZE];
  int count;
  while (fgets(line, sizeof(line), findex)) 
  {
    count++;
    strtok(line, "\n");
    printf("%s \n", line);
  }

  if (count == 1) 
  {
    printf("\nThere is %d file total.\n", count);
  } 
  else 
  {
    printf("\nThere are %d files total.\n", count);
  }

  return 0;
}

/* beargit rm <filename>
 *
 * See "Step 2" in the project spec.
 *
 */

int beargit_rm(const char* filename) 
{
  FILE* findex = fopen(".beargit/.index", "r");
  FILE *fnewindex = fopen(".beargit/.newindex", "w");

  int found = 0;

  char line[FILENAME_SIZE];
  while(fgets(line, sizeof(line), findex)) 
  {
    strtok(line, "\n");
    if (strcmp(line, filename) == 0) 
    {
      found = 1;
    } else 
    {
      fprintf(fnewindex, "%s\n", line);
    }
  }

  fclose(findex);
  fclose(fnewindex);

  if (found) {
    fs_mv(".beargit/.newindex", ".beargit/.index");
    return 0;
  } else {
    fprintf(stderr, "ERROR:  File %s not tracked.\n", filename);
    return 1;
  }
}

/* beargit commit -m <msg>
 *
 * See "Step 3" in the project spec.
 *
 */

const char* go_bears = "THIS IS BEAR TERRITORY!";

int is_commit_msg_ok(const char* msg) {
  char *msg_counter = msg;
  char *check_bears = go_bears;
  char *check_msg = msg_counter;
  while (*check_msg != 0)
  {
    int matching = 1;
    while (*check_bears != 0 && *check_msg != 0 && matching)
    {
      if (*check_msg != *check_bears)
      {
        matching = 0;
      }
      check_msg ++;
      check_bears ++;
    }
    if (matching == 1 && *check_bears == 0)
      return 1;
    msg_counter ++;
    check_msg = msg_counter;
    check_bears = go_bears;
  }
}

/* Use next_commit_id to fill in the rest of the commit ID.
 *
 * Hints:
 * You will need a destination string buffer to hold your next_commit_id, before you copy it back to commit_id
 * You will need to use a function we have provided for you.
 */

void next_commit_id(char* commit_id) {
     char next_id[COMMIT_ID_SIZE];
     char branch[BRANCHNAME_SIZE];
     read_string_from_file(".beargit/.current_branch", branch, BRANCHNAME_SIZE);
     char *new_name = malloc(strlen(commit_id) + strlen(branch) + 1);
     strcpy(new_name, commit_id);
     strcat(new_name, branch);
     cryptohash(new_name, next_id);
     strcpy(commit_id, next_id);
     free(new_name);
}

int at_branch_head() {
  char current_branch[BRANCHNAME_SIZE];
  read_string_from_file(".beargit/.current_branch", current_branch, BRANCHNAME_SIZE);
  return strlen(current_branch);
}

int beargit_commit(const char* msg) {
  if (!is_commit_msg_ok(msg)) {
    fprintf(stderr, "ERROR:  Message must contain \"%s\"\n", go_bears);
    return 1;
  } 
  else if (!at_branch_head()) {
    fprintf(stderr, "ERROR:  Need to be on HEAD of a branch to commit.\n");
    return 1;
  }

  char *commit_id = malloc(COMMIT_ID_SIZE + 1);
  read_string_from_file(".beargit/.prev", commit_id, COMMIT_ID_SIZE);
  next_commit_id(commit_id);

  //Make .beargit/<commit_id> directory
  char commit_dir[snprintf(NULL, 0, ".beargit/%s", commit_id) + 1];
  sprintf(commit_dir, ".beargit/%s", commit_id);
  fs_mkdir(commit_dir);

  //copy .beargit/.index file to .beargit/<commit_id>/.index
  char *index_dir = malloc(snprintf(NULL, 0, ".beargit/%s/.index", commit_id) + 1);
  sprintf(index_dir, ".beargit/%s/.index", commit_id);
  // fclose(fopen(index_dir, "w"));
  fs_cp(".beargit/.index", index_dir);//index_dir is the address for .beargit/commit_id/.index

  //make and write message into .beargit/<commit_id>/.msg
  char msg_dir[snprintf(NULL, 0, ".beargit/%s/.msg", commit_id) + 1];
  sprintf(msg_dir, ".beargit/%s/.msg", commit_id);
  FILE* msg_file = fopen(msg_dir, "w");
  fclose(msg_file);
  write_string_to_file(msg_dir, msg);

  //copy all files from .beargit/.index to .beargit/<commit_id>
  FILE* index_file = fopen(".beargit/.index", "r");
  char line[FILENAME_SIZE];
  while(fgets(line, sizeof(line), index_file))
  {
    strtok(line, "\n");
    char *new_file = malloc(snprintf(NULL, 0, ".beargit/%s/%s", commit_id, line) + 1);
    sprintf(new_file, ".beargit/%s/%s", commit_id, line);
    // FILE* temp = fopen(new_file, "w");
    // fclose(new_file);
    fs_cp(line, new_file);
    free((void*) new_file);
  }
  fclose(index_file);

  //copy .beargit/.prev to .beargit/<commit_id>/.prev
  char* prev = malloc(snprintf(NULL, 0, ".beargit/%s/.prev", commit_id) + 1);
  sprintf(prev, ".beargit/%s/.prev", commit_id);
  fs_cp(".beargit/.prev", prev);

  //write current commit_id to .beargit/.prev
  write_string_to_file(".beargit/.prev", commit_id);

<<<<<<< HEAD
=======
  printf("%s\n", commit_id);

>>>>>>> 181bb875d2dff2370d976bdc058c7231b4541cd6
  free((void*) index_dir);
  free((void *) prev);
  free((void *) commit_id);
  return 0;
}

/* beargit log
 *
 * See "Step 4" in the project spec.
 *
 */

int beargit_log(int limit) {
  char commit_id[COMMIT_ID_SIZE];
  read_string_from_file(".beargit/.prev", commit_id, COMMIT_ID_SIZE);
  int count = 0;
  if (at_first_commit(commit_id))
  {
    fprintf(stderr, "ERROR:  There are no commits.\n");
    return 1;
  }
  while (count < limit && !at_first_commit(commit_id))
  {
    char msg[MSG_SIZE];
    char msg_dir[FILENAME_SIZE];
    sprintf(msg_dir, ".beargit/%s/.msg", commit_id);
    read_string_from_file(msg_dir, msg, MSG_SIZE);
    fprintf(stdout, "commit %s\n   %s\n\n", commit_id, msg);
    char commit_dir[FILENAME_SIZE];
    sprintf(commit_dir, ".beargit/%s/.prev", commit_id);
    read_string_from_file(commit_dir, commit_id, COMMIT_ID_SIZE);
  }
  return 0;
}

int at_first_commit(char *commit_id)
{
  for(int i = 0; i < strlen(commit_id); i++)
    if (commit_id[i] != '0') 
      return 0;
  return 1;
}


// This helper function returns the branch number for a specific branch, or
// returns -1 if the branch does not exist.
int get_branch_number(const char* branch_name) {
  FILE* fbranches = fopen(".beargit/.branches", "r");

  int branch_index = -1;
  int counter = 0;
  char line[FILENAME_SIZE];
  while(fgets(line, sizeof(line), fbranches)) {
    strtok(line, "\n");
    if (strcmp(line, branch_name) == 0) {
      branch_index = counter;
    }
    counter++;
  }

  fclose(fbranches);

  return branch_index;
}

/* beargit branch
 *
 * See "Step 5" in the project spec.
 *
 */

int beargit_branch() {
  FILE* branches = fopen(".beargit/.branches", "r");
  FILE* current_branch = fopen(".beargit/.current_branch", "r"); 

  char line[FILENAME_SIZE];
  char current[FILENAME_SIZE];
  fgets(current, sizeof(current), current_branch);

  while(fgets(line, sizeof(line), branches)) {
    strtok(line, "\n");
    if (strcmp(line, current) == 0) {
      printf("*  %s\n", line);
    } else {
      printf("   %s\n", line);
    }
  }

  fclose(branches);
  fclose(current_branch);
  return 0;
}

/* beargit checkout
 *
 * See "Step 6" in the project spec.
 *
 */

int checkout_commit(const char* commit_id) {
<<<<<<< HEAD
  //Go through current .index file and remove all files from working directory
  char line[FILENAME_SIZE];
  FILE* current_index = fopen(".beargit/.index", "r");
=======
  char *current_index = malloc(snprintf(NULL, 0, ".beargit/.index") + 1);
  sprintf(current_index, ".beargit/.index");

  FILE *current_file = fopen(current_index, "r");

  char line[FILENAME_SIZE];

  while (fgets(line, sizeof(line), current_file)) {
    strtok(line, "\n");
    fs_rm(line);
  }

  fclose(current_file);

  free((void*) current_index);

  write_string_to_file(".beargit/.prev", commit_id);

  char *folder = malloc(snprintf(NULL, 0, ".beargit/%s", commit_id) + 1);
  sprintf(folder, ".beargit/%s", commit_id);

  char *index = malloc(snprintf(NULL, 0, ".beargit/%s/.index", commit_id) + 1);
  sprintf(index, ".beargit/%s/.index", commit_id);
  fs_cp(index, ".beargit/.index");

  FILE* index_file = fopen(index, "r");

  free((void*) folder);
  free((void*) index);

  if (index_file == NULL) {
    write_string_to_file(".beargit/.index", "");

    fclose(index_file);

    return 0;
  }
>>>>>>> 181bb875d2dff2370d976bdc058c7231b4541cd6

  while (fgets(line, sizeof(line), current_index))
  {
    strtok(line, "\n");
<<<<<<< HEAD
    FILE* file = fopen(line, "r");
    if (file != NULL)
      fs_rm(line);
=======
    char *new_file = malloc(snprintf(NULL, 0, ".beargit/%s/%s", commit_id, line) + 1);
    sprintf(new_file, ".beargit/%s/%s", commit_id, line);
    fs_cp(new_file, line);
    free((void*) new_file);
>>>>>>> 181bb875d2dff2370d976bdc058c7231b4541cd6
  }
  fclose(current_index);

  if (!at_first_commit(commit_id))
  {
    //copy index from commit being checked out to the .beargit/.index file
    char commit_index[FILENAME_SIZE];
    sprintf(commit_index, ".beargit/%s/.index", commit_id);
    fs_cp(commit_index, ".beargit/.index");

    //copy all files from the new index into the working directory from the checked out commit
    FILE* new_index = fopen(".beargit/.index", "r");
    char file_to_copy[FILENAME_SIZE];
    while(fgets(line, sizeof(line), new_index))
    {
      strtok(line, "\n");
      sprintf(file_to_copy, ".beargit/%s/%s", commit_id, line);
      FILE* new_file = fopen(line, "w");
      fclose(new_file);
      fs_cp(file_to_copy, line);
    }
    fclose(new_index);
  }
  else
  {
    FILE* new_index = fopen(".beargit/.newindex", "w");
    fclose(new_index);
    fs_mv(".beargit/.newindex", ".beargit/.index");
  }

  //write the ID of the checked out commit to .prev
  write_string_to_file(".beargit/.prev", commit_id);
  return 0;
}

int is_it_a_commit_id(const char* commit_id) {
  char beargit[9 + COMMIT_ID_SIZE] = ".beargit/";
  strcat(beargit, commit_id);

  if (access( beargit, F_OK ) != -1 || at_first_commit(commit_id)) {
    return 1;
  } else {
    return 0;
  }
}

int beargit_checkout(const char* arg, int new_branch) {
  // Get the current branch
  char current_branch[BRANCHNAME_SIZE];
  read_string_from_file(".beargit/.current_branch", current_branch, BRANCHNAME_SIZE);

  // If not detached, leave the current branch by storing the current HEAD into that branch's file...
  if (strlen(current_branch)) {
    char current_branch_file[FILENAME_SIZE];
    sprintf(current_branch_file, ".beargit/.branch_%s", current_branch);
    fs_cp(".beargit/.prev", current_branch_file);
  }

  printf("got here");

   // Check whether the argument is a commit ID. If yes, we just change to detached mode
  // without actually having to change into any other branch.
  if (is_it_a_commit_id(arg)) {
    char commit_dir[FILENAME_SIZE] = ".beargit/";
    strcat(commit_dir, arg);
    // ...and setting the current branch to none (i.e., detached).
    write_string_to_file(".beargit/.current_branch", "");

    return checkout_commit(arg);
  }



  // Read branches file (giving us the HEAD commit id for that branch).
  int branch_exists = (get_branch_number(arg) >= 0);

  printf("REACHED HERE");

  // Check for errors.
  if (branch_exists && new_branch) {
    fprintf(stderr, "ERROR:  A branch named %s already exists.\n", arg);
    return 1;
  } else if (!branch_exists && !new_branch) {
    fprintf(stderr, "ERROR:  No branch or commit %s exists.\n", arg);
    return 1;
  }

  // Just a better name, since we now know the argument is a branch name.
  const char* branch_name = arg;

  // File for the branch we are changing into.
  char branch_file[FILENAME_SIZE] = ".beargit/.branch_";
  strcat(branch_file, branch_name);

  printf("REACHED");

  // Update the branch file if new branch is created (now it can't go wrong anymore)
  if (new_branch) {
    FILE* fbranches = fopen(".beargit/.branches", "a");
    fprintf(fbranches, "%s\n", branch_name);
    fclose(fbranches);
    fs_cp(".beargit/.prev", branch_file);
  }
  write_string_to_file(".beargit/.current_branch", branch_name);

  // Read the head commit ID of this branch.
  char branch_head_commit_id[COMMIT_ID_SIZE];
  read_string_from_file(branch_file, branch_head_commit_id, COMMIT_ID_SIZE);

  printf(branch_head_commit_id);

  // Check out the actual commit.
  return checkout_commit(branch_head_commit_id);
}

/* beargit reset
 *
 * See "Step 7" in the project spec.
 *
 */

int beargit_reset(const char* commit_id, const char* filename) {
  if (!is_it_a_commit_id(commit_id)) {
      fprintf(stderr, "ERROR:  Commit %s does not exist.\n", commit_id);
      return 1;
  }

  // Check if the file is in the commit directory
  char file_to_reset[FILENAME_SIZE];
  sprintf(file_to_reset, ".beargit/%s/%s", commit_id, filename);
  FILE* file = fopen(file_to_reset, "r");
  if (file == NULL)
  {
    fprintf(stderr, "ERROR:  %s is not in the index of commit %s.\n", filename, commit_id);
    return 1;
  }
  fclose(file);

  // Copy the file to the current working directory
  fs_cp(file_to_reset, filename);

  // Add the file if it wasn't already there
  char line[FILENAME_SIZE];
  FILE* current_index = fopen(".beargit/.index", "r");
  while(fgets(line, sizeof(line), current_index))
  {
    strtok(line, "\n");
    if (strcmp(line, filename) == 0)
      return 0;
  }
  fclose(current_index);

  beargit_add(filename);

  return 0;
}

/* beargit merge
 *
 * See "Step 8" in the project spec.
 *
 */

int beargit_merge(const char* arg) {
  // Get the commit_id or throw an error
  char commit_id[COMMIT_ID_SIZE];
  if (!is_it_a_commit_id(arg)) {
      if (get_branch_number(arg) == -1) {
            fprintf(stderr, "ERROR:  No branch or commit %s exists.\n", arg);
            return 1;
      }
      char branch_file[FILENAME_SIZE];
      snprintf(branch_file, FILENAME_SIZE, ".beargit/.branch_%s", arg);
      read_string_from_file(branch_file, commit_id, COMMIT_ID_SIZE);
  } else {
      snprintf(commit_id, COMMIT_ID_SIZE, "%s", arg);
  }

  // Iterate through each line of the commit_id index and determine how you
  // should copy the index file over
  char index_file[FILENAME_SIZE];
  sprintf(index_file, ".beargit/%s/.index", commit_id);
  char line[FILENAME_SIZE];
  FILE* file = fopen(index_file, "r");
  while(fgets(line, sizeof(line), file))
  {
    strtok(line, "\n");
    if (is_being_tracked(line))
    {
      char new_filename[FILENAME_SIZE];
      sprintf(new_filename, "%s.%s", line, commit_id);
      char old_file[FILENAME_SIZE];
      sprintf(old_file, ".beargit/%s/%s", commit_id, line);
      fs_cp(old_file, new_filename);
      fprintf(stdout, "%s conflicted copy created\n", line);
    }
    else
    {
      char filename[FILENAME_SIZE];
      sprintf(filename, ".beargit/%s/%s", commit_id, line);
      fs_cp(filename, line);
      beargit_add(line);
      fprintf(stdout, "%s added\n", line);
    }
  }

  return 0;
}

//returns whether the file is being tracked in the .index directory
int is_being_tracked(char* filename)
{
  char line[FILENAME_SIZE];
  FILE* index = fopen(".beargit/.index", "r");
  while (fgets(line, sizeof(line), index))
  {
    strtok(line, "\n");
    if (strcmp(line, filename) == 0)
      return 1;
  }
  return 0;
}