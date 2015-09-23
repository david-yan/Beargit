#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <CUnit/Basic.h>
#include "beargit.h"
#include "util.h"

/* printf/fprintf calls in this tester will NOT go to file. */

#undef printf
#undef fprintf

/* The suite initialization function.
 * You'll probably want to delete any leftover files in .beargit from previous
 * tests, along with the .beargit directory itself.
 *
 * You'll most likely be able to share this across suites.
 */
int init_suite(void)
{
    // preps to run tests by deleting the .beargit directory if it exists
    fs_force_rm_beargit_dir();
    unlink("TEST_STDOUT");
    unlink("TEST_STDERR");
    return 0;
}

/* You can also delete leftover files after a test suite runs, but there's
 * no need to duplicate code between this and init_suite 
 */
int clean_suite(void)
{
    return 0;
}

/* Simple test of fread().
 * Reads the data previously written by testFPRINTF()
 * and checks whether the expected characters are present.
 * Must be run after testFPRINTF().
 */
void simple_sample_test(void)
{
    // This is a very basic test. Your tests should likely do more than this.
    // We suggest checking the outputs of printfs/fprintfs to both stdout
    // and stderr. To make this convenient for you, the tester replaces
    // printf and fprintf with copies that write data to a file for you
    // to access. To access all output written to stdout, you can read 
    // from the "TEST_STDOUT" file. To access all output written to stderr,
    // you can read from the "TEST_STDERR" file.
    int retval;
    retval = beargit_init();
    CU_ASSERT(0==retval);
    retval = beargit_add("asdf.txt");
    CU_ASSERT(0==retval);
}

struct commit {
  char msg[MSG_SIZE];
  struct commit* next;
};


void free_commit_list(struct commit** commit_list) {
  if (*commit_list) {
    free_commit_list(&((*commit_list)->next));
    free(*commit_list);
  }

  *commit_list = NULL;
}

void run_commit(struct commit** commit_list, const char* msg) {
    int retval = beargit_commit(msg);
    CU_ASSERT(0==retval);

    struct commit* new_commit = (struct commit*)malloc(sizeof(struct commit));
    new_commit->next = *commit_list;
    strcpy(new_commit->msg, msg);
    *commit_list = new_commit;
}

void simple_log_test(void)
{
    struct commit* commit_list = NULL;
    int retval;
    retval = beargit_init();
    CU_ASSERT(0==retval);
    FILE* asdf = fopen("asdf.txt", "w");
    fclose(asdf);
    retval = beargit_add("asdf.txt");
    CU_ASSERT(0==retval);
    run_commit(&commit_list, "THIS IS BEAR TERRITORY!1");
    run_commit(&commit_list, "THIS IS BEAR TERRITORY!2");
    run_commit(&commit_list, "THIS IS BEAR TERRITORY!3");


    retval = beargit_log(10);

    CU_ASSERT(0==retval);

    struct commit* cur_commit = commit_list;

    const int LINE_SIZE = 512;
    char line[LINE_SIZE];

    FILE* fstdout = fopen("TEST_STDOUT", "r");
    CU_ASSERT_PTR_NOT_NULL(fstdout);

    while (cur_commit != NULL) {
      char refline[LINE_SIZE];

      // First line is commit -- don't check the ID.
      CU_ASSERT_PTR_NOT_NULL(fgets(line, LINE_SIZE, fstdout));
      CU_ASSERT(!strncmp(line,"commit", strlen("commit")));

      // Second line is msg
      sprintf(refline, "   %s\n", cur_commit->msg);
      CU_ASSERT_PTR_NOT_NULL(fgets(line, LINE_SIZE, fstdout));
      CU_ASSERT_STRING_EQUAL(line, refline);

      // Third line is empty
      CU_ASSERT_PTR_NOT_NULL(fgets(line, LINE_SIZE, fstdout));
      CU_ASSERT(!strcmp(line,"\n"));

      cur_commit = cur_commit->next;
    }

    CU_ASSERT_PTR_NULL(fgets(line, LINE_SIZE, fstdout));

    // It's the end of output
    CU_ASSERT(feof(fstdout));
    fclose(fstdout);

    free_commit_list(&commit_list);
}

/**********
TEST COMMIT
***********/
void test_commit_1(void)
{
  int retval = beargit_init();
  CU_ASSERT(0 == retval);

  retval = beargit_commit("THIS IS NOT BEAR TERRITORY!");
  CU_ASSERT(retval != 0);
  const int LINE_SIZE = 512;
  char line[LINE_SIZE];

  stderr = fopen("TEST_STDERR", "r");
  CU_ASSERT_PTR_NOT_NULL(stderr);

  CU_ASSERT_PTR_NOT_NULL(fgets(line, LINE_SIZE, stderr));
  CU_ASSERT_STRING_EQUAL(line, "ERROR:  Message must contain \"THIS IS BEAR TERRITORY!\"\n");
  fclose(stderr);
}
void test_commit_2(void)
{
  int retval;
  retval = beargit_init();
  CU_ASSERT(retval == 0);

  FILE* text = fopen("a.txt", "w");
  fclose(text);
  retval = beargit_add("a.txt");
  CU_ASSERT(retval == 0);

  retval = beargit_commit("THIS IS BEAR TERRITORY!");
  CU_ASSERT(retval == 0);
}
void test_commit_messages(void)
{
  int retval = beargit_init();
  CU_ASSERT(retval == 0);

  retval = beargit_commit("THIS  IS BEAR TERRITORY!");
  CU_ASSERT(0 != retval);

  retval = beargit_commit("this is bear territory!");
  CU_ASSERT(0 != retval);

  retval = beargit_commit("THIS IS BEAR TERRITORY");
  CU_ASSERT(0 != retval);

  retval = beargit_commit("!THIS IS BEAR TERRITORY!");
  CU_ASSERT(0 == retval);

  retval = beargit_commit("THIS IS BEAR TERRITORY!1234");
  CU_ASSERT(0 == retval);

  retval = beargit_commit("THIS IS BEAR TERRITORY 1234 THIS IS BEAR TERRITORY! THIS IS NOT BEAR TERRITORY!");
  CU_ASSERT(0 == retval);

  FILE* fstderr = fopen("TEST_STDERR", "r");
  char line[512];
  CU_ASSERT_PTR_NOT_NULL(fstderr);
  CU_ASSERT_PTR_NOT_NULL(fgets(line, 512, fstderr));
  CU_ASSERT_STRING_EQUAL(line, "ERROR:  Message must contain \"THIS IS BEAR TERRITORY!\"\n");
  CU_ASSERT_PTR_NOT_NULL(fgets(line, 512, fstderr));
  CU_ASSERT_STRING_EQUAL(line, "ERROR:  Message must contain \"THIS IS BEAR TERRITORY!\"\n");
  CU_ASSERT_PTR_NOT_NULL(fgets(line, 512, fstderr));
  CU_ASSERT_STRING_EQUAL(line, "ERROR:  Message must contain \"THIS IS BEAR TERRITORY!\"\n");
  CU_ASSERT_PTR_NULL(fgets(line, 512, fstderr));
  fclose(fstderr);
}
void test_not_at_head(void)
{
  char first_id[512] = "5fe5991ffba74e3d74a71939068a32bcc4605121";
  int retval = beargit_init();
  CU_ASSERT(0 == retval);
  retval = beargit_commit("THIS IS BEAR TERRITORY!");
  CU_ASSERT(0 == retval);
  retval = beargit_checkout(first_id, 0);
  CU_ASSERT(0 == retval);
  retval = beargit_commit("THIS IS BEAR TERRITORY!");
  CU_ASSERT(0 != retval);
  char line[512];
  FILE* fstderr = fopen("TEST_STDERR", "r");
  CU_ASSERT_PTR_NOT_NULL(fstderr);
  CU_ASSERT_PTR_NOT_NULL(fgets(line, 512, fstderr));
  CU_ASSERT_STRING_EQUAL(line, "ERROR:  Need to be on HEAD of a branch to commit.\n");
  CU_ASSERT_PTR_NULL(fgets(line, 512, fstderr));
  fclose(fstderr);
}

/************
TEST CHECKOUT
*************/
void test_basic_checkout(void)
{
  int retval;
  retval = beargit_init();
  CU_ASSERT(retval == 0);

  FILE* a = fopen("a", "w");
  fclose(a);
  FILE* b = fopen("b", "w");
  fclose(b);
  FILE *c = fopen("c", "w");
  fclose(c);

  retval = beargit_checkout("branch", 1);
  CU_ASSERT(retval == 0);
  retval = beargit_checkout("master", 0);
  CU_ASSERT(retval == 0);

  retval = beargit_commit("THIS IS BEAR TERRITORY!");
  CU_ASSERT(retval == 0);

  beargit_add("a");
  retval = beargit_commit("THIS IS BEAR TERRITORY!");
  CU_ASSERT(retval == 0);

  beargit_add("b");
  beargit_add("c");
  retval = beargit_commit("THIS IS BEAR TERRITORY!");
  CU_ASSERT(retval == 0);

  retval = beargit_checkout("branch", 0);
  CU_ASSERT(retval == 0);

  a = fopen("a", "r");
  b = fopen("b", "r");
  c = fopen("c", "r");
  CU_ASSERT(a == NULL);
  CU_ASSERT(b == NULL);
  CU_ASSERT(c == NULL);
}
void test_checkout_id(void)
{
  int retval = beargit_init();
  CU_ASSERT(0 == retval);

  write_string_to_file("a", "1");
  FILE* b = fopen("b", "w");
  fclose(b);
  FILE* c = fopen("c", "w");
  fclose(c);

  beargit_add("a");
  beargit_commit("THIS IS BEAR TERRITORY!");
  char first_id[512];
  FILE* prev = fopen(".beargit/.prev", "r");
  CU_ASSERT_PTR_NOT_NULL(prev);
  CU_ASSERT_PTR_NOT_NULL(fgets(first_id, 512, prev));
  fclose(prev);

  write_string_to_file("a", "2");
  beargit_add("b");
  beargit_commit("THIS IS BEAR TERRITORY!");
  beargit_add("c");
  beargit_commit("THIS IS BEAR TERRITORY!");

  retval = beargit_checkout(first_id, 0);
  CU_ASSERT(0 == retval);
  FILE* a = fopen("a", "r");
  b = fopen("b", "r");
  c = fopen("c", "r");
  CU_ASSERT_PTR_NOT_NULL(a);
  CU_ASSERT_PTR_NULL(b);
  CU_ASSERT_PTR_NULL(c);
  char line[512];
  CU_ASSERT_PTR_NOT_NULL(fgets(line, 512, a));
  CU_ASSERT_STRING_EQUAL(line, "1");
  fclose(a);
}
void test_checkout_input(void)
{
  beargit_init();
  
  //try to checkout nonexistant branch
  int retval = beargit_checkout("branch", 0);
  CU_ASSERT(0 != retval);

  //try to checkout a filename
  FILE* a = fopen("a", "w");
  fclose(a);
  retval = beargit_checkout("a", 0);
  CU_ASSERT(0 != retval);

  //try to make the same branch twice
  retval = beargit_checkout("branch", 1);
  CU_ASSERT(0 == retval);
  beargit_checkout("master", 0);
  retval = beargit_checkout("branch", 1);
  CU_ASSERT(0 != retval);

  //try same as above but at different commit and different branch
  beargit_checkout("other", 1);
  beargit_commit("THIS IS BEAR TERRITORY!");
  retval = beargit_checkout("branch", 1);
  CU_ASSERT(0 != retval);

  //error messages
  FILE* fstderr = fopen("TEST_STDERR", "r");
  char line[512];
  fgets(line, 512, fstderr);
  CU_ASSERT_STRING_EQUAL(line, "ERROR:  No branch or commit branch exists.\n");
  fgets(line, 512, fstderr);
  CU_ASSERT_STRING_EQUAL(line, "ERROR:  No branch or commit a exists.\n");
  fgets(line, 512, fstderr);
  CU_ASSERT_STRING_EQUAL(line, "ERROR:  A branch named branch already exists.\n");
  fgets(line, 512, fstderr);
  CU_ASSERT_STRING_EQUAL(line, "ERROR:  A branch named branch already exists.\n");
  CU_ASSERT_PTR_NULL(fgets(line, 512, fstderr));
  fclose(fstderr);
}
void test_checkout_id_from_other_branch(void)
{
  beargit_init();
  write_string_to_file("a", "right");
  FILE* b = fopen("b", "w");
  fclose(b);
  beargit_checkout("branch", 1);
  beargit_add("a");
  beargit_commit("THIS IS BEAR TERRITORY!");

  char id_to_checkout[512];
  FILE* prev = fopen(".beargit/.prev", "r");
  fgets(id_to_checkout, 512, prev);
  fclose(prev);

  write_string_to_file("a", "wrong");
  beargit_commit("THIS IS BEAR TERRITORY!");

  beargit_checkout("master", 0);
  write_string_to_file("a", "wrong");
  beargit_add("b");
  beargit_commit("THIS IS BEAR TERRITORY!");

  int retval = beargit_checkout(id_to_checkout, 0);
  CU_ASSERT(0 == retval);
  FILE* a = fopen("a", "r");
  b = fopen("b", "r");
  CU_ASSERT_PTR_NOT_NULL(a);
  CU_ASSERT_PTR_NULL(b);
  char line[512];
  CU_ASSERT_PTR_NOT_NULL(fgets(line, 512, a));
  CU_ASSERT_STRING_EQUAL(line, "right");
  fclose(a);
}
void test_checkout_0_commit(void)
{
  beargit_init();
  beargit_commit("THIS IS BEAR TERRITORY!");
  beargit_commit("THIS IS BEAR TERRITORY!");
  beargit_checkout("branch", 1);
  FILE* a = fopen("a", "w");
  fclose(a);
  beargit_add("a");
  beargit_commit("THIS IS BEAR TERRITORY!");
  beargit_checkout("master", 0);
  beargit_commit("THIS IS BEAR TERRITORY!");
  beargit_commit("THIS IS BEAR TERRITORY!");

  int retval = beargit_checkout("000", 0);
  CU_ASSERT(0 == retval);
  a = fopen("a", "r");
  CU_ASSERT_PTR_NULL(a);
}

/**********
TEST REMOVE
***********/
void test_basic_remove(void)
{
  int retval;
  retval = beargit_init();
  CU_ASSERT(0 == retval);

  FILE *a = fopen("a", "w");
  fclose(a);

  retval = beargit_add("a");
  CU_ASSERT(0 == retval);

  FILE* index = fopen(".beargit/.index", "r");
  CU_ASSERT_PTR_NOT_NULL(index);
  char line[512];
  CU_ASSERT_PTR_NOT_NULL(fgets(line, 512, index));
  fclose(index);
  strtok(line, "\n");
  CU_ASSERT_STRING_EQUAL(line, "a");

  retval = beargit_rm("a");
  CU_ASSERT(0 == retval);
  index = fopen(".beargit/.index", "r");
  CU_ASSERT_PTR_NOT_NULL(index);
  CU_ASSERT_PTR_NULL(fgets(line, 512, index));
  fclose(index);
}
void test_remove_errors_1(void)
{
  int retval = beargit_init();
  CU_ASSERT(0 == retval);

  //try removing when there is nothing in .index
  retval = beargit_rm("a");
  CU_ASSERT(0 != retval);
  FILE* fstderr = fopen("TEST_STDERR", "r");
  CU_ASSERT_PTR_NOT_NULL(fstderr);
  char line[512];
  CU_ASSERT_PTR_NOT_NULL(fgets(line, 512, fstderr))
  CU_ASSERT_STRING_EQUAL(line, "ERROR:  File a not tracked.\n");
  CU_ASSERT_PTR_NULL(fgets(line, 512, fstderr));
  fclose(fstderr);
}
void test_remove_errors_2(void)
{
  int retval = beargit_init();
  CU_ASSERT(0 == retval);

  FILE *a = fopen("a", "w");
  fclose(a);

  //try removing same file twice
  retval = beargit_add("a");
  CU_ASSERT(0 == retval);
  retval = beargit_rm("a");
  CU_ASSERT(0 == retval);
  retval = beargit_rm("a");
  CU_ASSERT(0 != retval);
  FILE* fstderr = fopen("TEST_STDERR", "r");
  char line[512];
  CU_ASSERT_PTR_NOT_NULL(fstderr);
  CU_ASSERT_PTR_NOT_NULL(fgets(line, 512, fstderr));
  CU_ASSERT_STRING_EQUAL(line,  "ERROR:  File a not tracked.\n");
  CU_ASSERT_PTR_NULL(fgets(line, 512, fstderr));
  fclose(fstderr);
}
void test_remove_errors_3(void)
{
  int retval = beargit_init();
  CU_ASSERT(0 == retval);

  FILE* a = fopen("a", "w");
  fclose(a);
  FILE* b = fopen("b", "w");
  fclose(b);

  //try removing file from other commit
  retval = beargit_add("a");
  CU_ASSERT(0 == retval);
  retval = beargit_add("b");
  CU_ASSERT(0 == retval);
  retval = beargit_commit("THIS IS BEAR TERRITORY!");
  CU_ASSERT(0 == retval);
  retval = beargit_rm("a");
  CU_ASSERT(0 == retval);
  retval = beargit_commit("THIS IS BEAR TERRITORY!");
  CU_ASSERT(0 == retval);
  retval = beargit_rm("a");
  CU_ASSERT(0 != retval);
  retval = beargit_rm("b");
  CU_ASSERT(0 == retval);
  FILE* fstderr = fopen("TEST_STDERR", "r");
  char line[512];
  CU_ASSERT_PTR_NOT_NULL(fstderr);
  CU_ASSERT_PTR_NOT_NULL(fgets(line, 512, fstderr));
  CU_ASSERT_STRING_EQUAL(line,  "ERROR:  File a not tracked.\n");
  CU_ASSERT_PTR_NULL(fgets(line, 512, fstderr));
  fclose(fstderr);
}
void test_remove_errors_4(void)
{
  int retval = beargit_init();
  CU_ASSERT(0 == retval);

  FILE* a = fopen("a", "w");
  fclose(a);
  FILE* b = fopen("b", "w");
  fclose(b);

  //try removing file after checking out
  retval = beargit_add("a");
  CU_ASSERT(0 == retval);
  retval = beargit_commit("THIS IS BEAR TERRITORY!");
  CU_ASSERT(0 == retval);
  retval = beargit_checkout("first", 1);
  CU_ASSERT(0 == retval);
  retval = beargit_checkout("master", 0);
  CU_ASSERT(0 == retval);
  retval = beargit_add("b");
  CU_ASSERT(0 == retval);
  retval = beargit_commit("THIS IS BEAR TERRITORY!");
  CU_ASSERT(0 == retval);
  retval = beargit_checkout("first", 0);
  CU_ASSERT(0 == retval);
  retval = beargit_rm("a");
  CU_ASSERT(0 == retval);
  retval = beargit_rm("b");
  CU_ASSERT(0 != retval);
  FILE* fstderr = fopen("TEST_STDERR", "r");
  char line[512];
  CU_ASSERT_PTR_NOT_NULL(fstderr);
  CU_ASSERT_PTR_NOT_NULL(fgets(line, 512, fstderr));
  CU_ASSERT_STRING_EQUAL(line,  "ERROR:  File b not tracked.\n");
  CU_ASSERT_PTR_NULL(fgets(line, 512, fstderr));
  fclose(fstderr);
}

/*************
**TEST RESET**
**************/
void test_basic_reset(void)
{
  beargit_init();
  write_string_to_file("a", "right");
  beargit_add("a");
  beargit_commit("THIS IS BEAR TERRITORY!");

  char first_id[512];
  FILE* prev = fopen(".beargit/.prev", "r");
  fgets(first_id, 512, prev);
  fclose(prev);

  write_string_to_file("a", "wrong 1");
  beargit_commit("THIS IS BEAR TERRITORY!");

  write_string_to_file("a", "wrong 2");
  beargit_commit("THIS IS BEAR TERRITORY!");

  int retval = beargit_reset(first_id, "a");
  CU_ASSERT(0 == retval);
  char line[512];
  read_string_from_file("a", line, 512);
  CU_ASSERT_STRING_EQUAL(line, "right");
}
void test_reset_error_cases(void)
{
  beargit_init();
  FILE* a = fopen("a", "w");
  fclose(a);
  beargit_commit("THIS IS BEAR TERRITORY!");
  char first_id[512];
  FILE* prev = fopen(".beargit/.prev", "r");
  fgets(first_id, 512, prev);
  fclose(prev);

  int retval = beargit_reset(first_id, "a");
  CU_ASSERT(0 != retval);
  
  beargit_add("a");
  beargit_commit("THIS IS BEAR TERRITORY!");
  char second_id[512];
  prev = fopen(".beargit/.prev", "r");
  fgets(second_id, 512, prev);
  fclose(prev);

  retval = beargit_reset("a", second_id);
  CU_ASSERT(0 != retval);

  char error[512];
  FILE* fstderr = fopen("TEST_STDERR", "r");
  fgets(error, 512, fstderr);
  char error1[512];
  sprintf(error1, "ERROR:  a is not in the index of commit %s.\n", first_id);
  CU_ASSERT_STRING_EQUAL(error, error1);
  fgets(error, 512, fstderr);
  CU_ASSERT_STRING_EQUAL(error, "ERROR:  Commit a does not exist.\n");
  fclose(fstderr);
}

/* The main() function for setting up and running the tests.
 * Returns a CUE_SUCCESS on successful running, another
 * CUnit error code on failure.
 */
int cunittester()
{
    CU_pSuite pSuite = NULL;
    CU_pSuite pSuite2 = NULL;
    //This set of suites tests the remove method. It tests the basic functionality
    //of the function and the error cases. The errors tested involve removing an invalid file
    CU_pSuite remove_test_1 = NULL;
    CU_pSuite remove_error_tests_1 = NULL;
    CU_pSuite remove_error_tests_2 = NULL;
    CU_pSuite remove_error_tests_3 = NULL;
    //This set of suites tests commit. It tests the functionality and the error cases of
    //having a bad commit message and committing when not at the head commit
    CU_pSuite commit_tests_1 = NULL;
    CU_pSuite commit_tests_2 = NULL;
    CU_pSuite commit_messages = NULL;
    //This set tests the functionality of checkout. 
    CU_pSuite checkout_test_input = NULL;
    CU_pSuite checkout_test_id_from_other = NULL;
    CU_pSuite checkout_test_0_commit = NULL;
    CU_pSuite reset_test_basic = NULL;
    CU_pSuite reset_test_errors = NULL;

    /* initialize the CUnit test registry */
    if (CUE_SUCCESS != CU_initialize_registry())
      return CU_get_error();

    /* add a suite to the registry */
    pSuite = CU_add_suite("Suite_1", init_suite, clean_suite);
    if (NULL == pSuite) {
      CU_cleanup_registry();
      return CU_get_error();
    }

    /* Add tests to the Suite #1 */
    if (NULL == CU_add_test(pSuite, "Simple Test #1", simple_sample_test))
    {
      CU_cleanup_registry();
      return CU_get_error();
    }

   /* Add tests to the Suite #2 */
   // if (NULL == CU_add_test(pSuite2, "Log output test", simple_log_test))
   // {
   //    CU_cleanup_registry();
   //    return CU_get_error();
   // }

    pSuite2 = CU_add_suite("Suite_2", init_suite, clean_suite);
    if (NULL == pSuite2) {
      CU_cleanup_registry();
      return CU_get_error();
    }
    if (NULL == CU_add_test(pSuite2, "Simple Log Test ", simple_log_test)) 
    {
       CU_cleanup_registry();
       return CU_get_error();
    }

    remove_test_1 = CU_add_suite("Remove Test #1", init_suite, clean_suite);
    if (NULL == remove_test_1)
    {
      CU_cleanup_registry();
      return CU_get_error();
    }
    if (NULL == CU_add_test(remove_test_1, "Basic Remove Test", test_basic_remove))
    {
      CU_cleanup_registry();
      return CU_get_error();
    }

    remove_error_tests_1 = CU_add_suite("Remove Error Tests", init_suite, clean_suite);
    if (NULL == remove_error_tests_1)
    {
      CU_cleanup_registry();
      return CU_get_error();
    }
    if (NULL == CU_add_test(remove_error_tests_1, "Remove Error Checking #1", test_remove_errors_1))
    {
      CU_cleanup_registry();
      return CU_get_error();
    }

    remove_error_tests_2 = CU_add_suite("Remove Error Tests", init_suite, clean_suite);
    if (NULL == remove_error_tests_2)
    {
      CU_cleanup_registry();
      return CU_get_error();
    }
    if (NULL == CU_add_test(remove_error_tests_2, "Remove Error Checking #2", test_remove_errors_2))
    {
      CU_cleanup_registry();
      return CU_get_error();
    }

    remove_error_tests_3 = CU_add_suite("Remove Error Tests", init_suite, clean_suite);
    if (NULL == remove_error_tests_3)
    {
      CU_cleanup_registry();
      return CU_get_error();
    }
    if (NULL == CU_add_test(remove_error_tests_3, "Remove Error Checking #3", test_remove_errors_3))
    {
      CU_cleanup_registry();
      return CU_get_error();
    }

    commit_tests_1 = CU_add_suite("Commit Test #1", init_suite, clean_suite);
    if (NULL == commit_tests_1)
    {
      CU_cleanup_registry();
      return CU_get_error();
    }
    if (NULL == CU_add_test(commit_tests_1, "Commit Test #1", test_commit_1))
    {
      CU_cleanup_registry();
      return CU_get_error();
    }

    commit_tests_2 = CU_add_suite("Commit Test #2", init_suite, clean_suite);
    if (NULL == commit_tests_2)
    {
      CU_cleanup_registry();
      return CU_get_error();
    }
    if (NULL == CU_add_test(commit_tests_2, "Commit Test #2", test_commit_2))
    {
      CU_cleanup_registry();
      return CU_get_error();
    }

    commit_messages = CU_add_suite("Commit Message Tests", init_suite, clean_suite);
    if (NULL == commit_messages)
    {
      CU_cleanup_registry();
      return CU_get_error();
    }
    if (NULL == CU_add_test(commit_messages, "Check Commit Messages", test_commit_messages))
    {
      CU_cleanup_registry();
      return CU_get_error();
    }

    checkout_test_input = CU_add_suite("Checkout Tests", init_suite, clean_suite);
    if (NULL == checkout_test_input)
    {
      CU_cleanup_registry();
      return CU_get_error();
    }
    if (NULL == CU_add_test(checkout_test_input, "Checkout input test", test_checkout_input))
    {
      CU_cleanup_registry();
      return CU_get_error();
    }

    checkout_test_id_from_other = CU_add_suite("Checkout Tests", init_suite, clean_suite);
    if (NULL == checkout_test_id_from_other)
    {
      CU_cleanup_registry();
      return CU_get_error();
    }
    if (NULL == CU_add_test(checkout_test_id_from_other, "Checkout commit ID from other branch test", test_checkout_id_from_other_branch))
    {
      CU_cleanup_registry();
      return CU_get_error();
    }

    checkout_test_0_commit = CU_add_suite("Checkout Tests", init_suite, clean_suite);
    if (NULL == checkout_test_0_commit)
    {
      CU_cleanup_registry();
      return CU_get_error();
    }
    if (NULL == CU_add_test(checkout_test_0_commit, "Checkout 0 ID test", test_checkout_0_commit))
    {
      CU_cleanup_registry();
      return CU_get_error();
    }

    reset_test_basic = CU_add_suite("Checkout Tests", init_suite, clean_suite);
    if (NULL == reset_test_basic)
    {
      CU_cleanup_registry();
      return CU_get_error();
    }
    if (NULL == CU_add_test(reset_test_basic, "Basic reset test", test_basic_reset))
    {
      CU_cleanup_registry();
      return CU_get_error();
    }

    reset_test_errors = CU_add_suite("Checkout Tests", init_suite, clean_suite);
    if (NULL == reset_test_errors)
    {
      CU_cleanup_registry();
      return CU_get_error();
    }
    if (NULL == CU_add_test(reset_test_errors, "Basic reset test", test_reset_error_cases))
    {
      CU_cleanup_registry();
      return CU_get_error();
    }

    /* Run all tests using the CUnit Basic interface */
    CU_basic_set_mode(CU_BRM_VERBOSE);
    CU_basic_run_tests();
    CU_cleanup_registry();
    return CU_get_error();
}

