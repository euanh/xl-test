int
main(int argc __attribute__((unused)), 
     char **argv __attribute__((unused)))
{
       struct test t;
       test_init(&t);
       test_spawn(&t, testcase);

       send_fd_event(&t, 0);
       send_fd_event(&t, 1);
       send_fd_event(&t, 2);
       send_fd_event(&t, 3);
       send_fd_event(&t, 4);

       send_libxl_callback_event(&t);
       send_libxl_callback_event(&t);

       send_eventloop(&t);

       pthread_join(t.thread, NULL);
       test_destroy(&t);
       pthread_exit(NULL);
}

