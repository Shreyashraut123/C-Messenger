#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

// Conditional compilation for terminal control
#ifdef _WIN32
    #include <conio.h>
    #define CLEAR_SCREEN "cls"
#else
    #include <termios.h>
    #define CLEAR_SCREEN "clear"
#endif

#define MAX_USERNAME_LENGTH 50
#define MAX_MESSAGE_LENGTH 500
#define MAX_MESSAGES 100
#define MAX_USERS 10

// Conditional color support
#ifdef _WIN32
    #define RESET ""
    #define BLUE ""
    #define GREEN ""
    #define YELLOW ""
    #define CYAN ""
    #define RED ""
#else
    #define RESET "\x1b[0m"
    #define BLUE "\x1b[34m"
    #define GREEN "\x1b[32m"
    #define YELLOW "\x1b[33m"
    #define CYAN "\x1b[36m"
    #define RED "\x1b[31m"
#endif

// User Structure
typedef struct User {
    char username[MAX_USERNAME_LENGTH];
    int user_id;
    int active;
} User;

// Message Structure
typedef struct Message {
    User sender;
    char content[MAX_MESSAGE_LENGTH];
    time_t timestamp;
    struct Message* next;
} Message;

// Chat Room Structure
typedef struct {
    Message* front;
    Message* rear;
    int message_count;
    User users[MAX_USERS];
    int current_user_index;
} ChatRoom;

// Platform-independent getch
int safe_getch() {
    #ifdef _WIN32
        return _getch();
    #else
        struct termios oldt, newt;
        int ch;
        tcgetattr(STDIN_FILENO, &oldt);
        newt = oldt;
        newt.c_lflag &= ~(ICANON | ECHO);
        tcsetattr(STDIN_FILENO, TCSANOW, &newt);
        ch = getchar();
        tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
        return ch;
    #endif
}

// Clear Screen Function
void clearScreen() {
    system(CLEAR_SCREEN);
}

// Function Prototypes
ChatRoom* createChatRoom();
void sendMessage(ChatRoom* chatRoom, User sender, const char* content);
void displayMessages(ChatRoom* chatRoom);
void saveMessagesToFile(ChatRoom* chatRoom, const char* filename);
void loadMessagesFromFile(ChatRoom* chatRoom, const char* filename);
void freeMessages(ChatRoom* chatRoom);

// Create a new chat room
ChatRoom* createChatRoom() {
    ChatRoom* newChatRoom = (ChatRoom*)malloc(sizeof(ChatRoom));
    if (newChatRoom == NULL) {
        printf("Memory allocation failed!\n");
        exit(1);
    }
    int i ;
    // Initialize users array
    for ( i = 0; i < MAX_USERS; i++) {
        newChatRoom->users[i].active = 0;
    }
    
    newChatRoom->front = NULL;
    newChatRoom->rear = NULL;
    newChatRoom->message_count = 0;
    newChatRoom->current_user_index = -1;
    
    return newChatRoom;
}

// Send a message to the chat room
void sendMessage(ChatRoom* chatRoom, User sender, const char* content) {
    if (chatRoom->message_count >= MAX_MESSAGES) {
        // Remove the oldest message if limit is reached
        Message* temp = chatRoom->front;
        chatRoom->front = chatRoom->front->next;
        free(temp);
        chatRoom->message_count--;
    }

    // Create new message
    Message* newMessage = (Message*)malloc(sizeof(Message));
    if (newMessage == NULL) {
        printf("Memory allocation failed!\n");
        exit(1);
    }

    // Copy sender info and message content
    strcpy(newMessage->sender.username, sender.username);
    newMessage->sender.user_id = sender.user_id;
    strcpy(newMessage->content, content);

    // Set timestamp
    newMessage->timestamp = time(NULL);
    newMessage->next = NULL;

    // Add to chat room
    if (chatRoom->rear == NULL) {
        chatRoom->front = newMessage;
        chatRoom->rear = newMessage;
    } else {
        chatRoom->rear->next = newMessage;
        chatRoom->rear = newMessage;
    }

    chatRoom->message_count++;
}

// Display all messages in the chat room
void displayMessages(ChatRoom* chatRoom) {
    Message* current = chatRoom->front;

    printf("\n+------------------------------------------------+\n");
    printf("|              CHAT HISTORY                      |\n");
    printf("+------------------------------------------------+\n");

    if (current == NULL) {
        printf("|              No messages yet                  |\n");
    } else {
        while (current != NULL) {
            char* timeStr = ctime(&current->timestamp);
            timeStr[strlen(timeStr) - 1] = '\0';  // Remove newline

            printf("| [%s] %s (ID: %d):\n| %s\n",
                   timeStr,
                   current->sender.username,
                   current->sender.user_id,
                   current->content);

            printf("+------------------------------------------------+\n");
            current = current->next;
        }
    }
}

// Save messages to a file
void saveMessagesToFile(ChatRoom* chatRoom, const char* filename) {
    FILE* file = fopen(filename, "w");
    if (file == NULL) {
        printf("Error opening file for writing!\n");
        return;
    }

    Message* current = chatRoom->front;
    while (current != NULL) {
        fprintf(file, "%s|%d|%ld|%s\n",
                current->sender.username,
                current->sender.user_id,
                current->timestamp,
                current->content);
        current = current->next;
    }

    fclose(file);
    printf("✅ Chat history saved successfully!\n");
}

// Load messages from a file
void loadMessagesFromFile(ChatRoom* chatRoom, const char* filename) {
    FILE* file = fopen(filename, "r");
    if (file == NULL) {
        printf("No previous chat history found.\n");
        return;
    }

    // Clear existing messages
    freeMessages(chatRoom);

    char line[1024];
    while (fgets(line, sizeof(line), file)) {
        User sender;
        time_t timestamp;
        char content[MAX_MESSAGE_LENGTH];

        // Corrected format string
        if (sscanf(line, "%49[^|]|%d|%ld|%499[^\n]",
                   sender.username, &sender.user_id, &timestamp, content) == 4) {
            Message* newMessage = (Message*)malloc(sizeof(Message));
            if (newMessage == NULL) {
                printf("Memory allocation failed!\n");
                exit(1);
            }

            strcpy(newMessage->sender.username, sender.username);
            newMessage->sender.user_id = sender.user_id;
            strcpy(newMessage->content, content);
            newMessage->timestamp = timestamp;
            newMessage->next = NULL;

            if (chatRoom->rear == NULL) {
                chatRoom->front = newMessage;
                chatRoom->rear = newMessage;
            } else {
                chatRoom->rear->next = newMessage;
                chatRoom->rear = newMessage;
            }
            chatRoom->message_count++;
        }
    }

    fclose(file);
    printf("✅ Chat history loaded successfully!\n");
}

// Free all messages in the chat room
void freeMessages(ChatRoom* chatRoom) {
    while (chatRoom->front != NULL) {
        Message* temp = chatRoom->front;
        chatRoom->front = chatRoom->front->next;
        free(temp);
    }
    chatRoom->rear = NULL;
    chatRoom->message_count = 0;
}

// Add New User Function
void addNewUser(ChatRoom* chatRoom) {
    for (int i = 0; i < MAX_USERS; i++) {
        if (!chatRoom->users[i].active) {
            printf("Enter new username: ");
            fgets(chatRoom->users[i].username, 
                  sizeof(chatRoom->users[i].username), stdin);
            chatRoom->users[i].username[strcspn(
                chatRoom->users[i].username, "\n")] = 0;
            
            chatRoom->users[i].user_id = rand() % 1000 + 1;
            chatRoom->users[i].active = 1;
            
            chatRoom->current_user_index = i;
            
            printf("User added successfully!\n");
            return;
        }
    }
    printf("Maximum users reached!\n");
}

// Display Available Users
void displayUsers(ChatRoom* chatRoom) {
    clearScreen();
    printf("Available Users:\n");
    printf("+----+----------------------+--------+\n");
    printf("| No | Username            | User ID |\n");
    printf("+----+----------------------+--------+\n");
    
    for (int i = 0; i < MAX_USERS; i++) {
        if (chatRoom->users[i].active) {
            printf("| %2d | %-20s | %6d |\n", 
                   i+1, 
                   chatRoom->users[i].username, 
                   chatRoom->users[i].user_id);
        }
    }
    
    printf("+----+----------------------+--------+\n");
}

// Switch User Function
void switchUser(ChatRoom* chatRoom) {
    displayUsers(chatRoom);
    
    printf("\nEnter user number to switch: ");
    int choice;
    scanf("%d", &choice);
    getchar();  // Consume newline

    if (choice > 0 && choice <= MAX_USERS && 
        chatRoom->users[choice-1].active) {
        chatRoom->current_user_index = choice - 1;
        printf("Switched user successfully!\n");
    } else {
        printf("Invalid user selection!\n");
    }
}

int main() {
    // Seed random number generator
    srand(time(NULL));
    
    // Create chat room
    ChatRoom* chatRoom = createChatRoom();
    
    // Add first user
    addNewUser(chatRoom);
    
    // Load previous chat history
    loadMessagesFromFile(chatRoom, "chat_history.txt");

    // Message variables
    char message[MAX_MESSAGE_LENGTH];
    int continueChat = 1;

    while (continueChat) {
        clearScreen();
        printf("C-CHAT APP\n");
        printf("Current User: %s (ID: %d)\n\n", 
               chatRoom->users[chatRoom->current_user_index].username,
               chatRoom->users[chatRoom->current_user_index].user_id);
        
        printf("+----------------------------+\n");
        printf("|        MAIN MENU           |\n");
        printf("+----------------------------+\n");
        printf("1. Send Message\n");
        printf("2. View Chat History\n");
        printf("3. Switch User\n");
        printf("4. Add New User\n");
        printf("5. Save Chat History\n");
        printf("6. Exit Chat\n");
        printf("+----------------------------+\n");
        printf("Choose an option: ");

        int choice;
        scanf("%d", &choice);
        getchar();  // Consume newline

        switch (choice) {
            case 1: {
                while (1) {
                    clearScreen();
                    printf("%s, enter your message (type 'exit' to stop):\n", 
                           chatRoom->users[chatRoom->current_user_index].username);

                    fgets(message, sizeof(message), stdin);
                    message[strcspn(message, "\n")] = 0;  // Remove newline

                    if (strcmp(message, "exit") == 0) {
                        break;
                    }

                    sendMessage(chatRoom, 
                                chatRoom->users[chatRoom->current_user_index], 
                                message);
                    printf("Message sent successfully!\n");
                    sleep(1);
                }
                break;
            }

            case 2:
                displayMessages(chatRoom);
                printf("\nPress Enter to continue...");
                getchar();
                break;

            case 3:
                switchUser(chatRoom);
                break;

            case 4:
                addNewUser(chatRoom);
                break;

            case 5:
                saveMessagesToFile(chatRoom, "chat_history.txt");
                printf("Chat history saved!\n");
                sleep(1);
                break;

            case 6:
                printf("Exiting C-Chat App. Goodbye, %s!\n", 
                       chatRoom->users[chatRoom->current_user_index].username);
                continueChat = 0;
                break;

            default:
                printf("Invalid option. Try again.\n");
                sleep(1);
        }
    }

    // Save chat history and clean up
    saveMessagesToFile(chatRoom, "chat_history1.txt");
    freeMessages(chatRoom);
    free(chatRoom);

    return 0;
}
