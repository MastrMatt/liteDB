# test the client with the server running
import os
import time
import unittest
import subprocess


class TestClientIntegration(unittest.TestCase):
    # run once before all tests are executed
    @classmethod
    def setUpClass(cls):
        # delete AOF.aof file if it exists
        if os.path.exists("./AOF.aof"):
            os.remove("./AOF.aof")

        # run the build for make all in ./client and ./server
        subprocess.run(["make", "all"], cwd="../client")
        subprocess.run(["make", "all"], cwd="../server")

        # Start the server
        cls.server_process = subprocess.Popen(
            ["../server/runserver", "-d"],
            stdin=subprocess.PIPE,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True,
        )

        # Wait for the server
        time.sleep(0.5)

        # Start the client process
        cls.client_process = subprocess.Popen(
            ["../client/runclient", "-d"],
            stdin=subprocess.PIPE,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            text=True,
        )

        # Wait for the client
        time.sleep(0.5)

    # run once after all tests are completed
    @classmethod
    def tearDownClass(cls):
        # Close the server
        cls.server_process.terminate()

        # Close the client
        cls.client_process.terminate()

    def test_client_server(self):
        # Send a simple set cmd to the server
        self.client_process.stdin.write("set key1 value1\n")
        self.client_process.stdin.flush()

        # give the server time to process the command
        time.sleep(0.1)

        # Read the startup message of the client and discard it
        response = self.client_process.stdout.readline()

        # Read the response of the server
        response = self.client_process.stdout.readline().strip()
        expected_response = "liteDB> (nil)"

        self.assertEqual(response, expected_response)

        # Send a simple get cmd to the server
        self.client_process.stdin.write("get key1\n")
        self.client_process.stdin.flush()

        # give the server time to process the command
        time.sleep(0.1)

        # Read the response of the server
        response = self.client_process.stdout.readline().strip()
        expected_response = "liteDB> (str) value1"

        self.assertEqual(response, expected_response)

        # Send a simple del cmd to the server
        self.client_process.stdin.write("del key1\n")
        self.client_process.stdin.flush()

        # give the server time to process the command
        time.sleep(0.1)

        # Read the response of the server
        response = self.client_process.stdout.readline().strip()
        expected_response = "liteDB> (int) 1"

        self.assertEqual(response, expected_response)


if __name__ == "__main__":
    unittest.main()
