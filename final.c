#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/glut.h>
#include <time.h>
#include <math.h>


int Ants_num;
int Food_Pieces;
int FOOD_INTERVAL;
int FOOD_RADIUS;
int LIGHT_PHEROMONE_THRESHOLD;
int STRONG_PHEROMONE_THRESHOLD;
int SIMULATION_TIME;
#define M_PI 3.14
#define MAX_BUFFER_SIZE 100


int food_num = 0;
int food_eaten = 0;
pthread_mutex_t *mutexes;
pthread_mutex_t cancel_mutex=PTHREAD_MUTEX_INITIALIZER;

int isFinished = 0;


typedef struct
{
    double x;
    double y;
    double angle;
    int speed;
    double prevX;
    double prevY;
    double foodX;
    double foodY;
    double pheromone;
    int seed;
} Ant;

typedef struct
{
    double x;
    double y;
    double portion;
    int seed;
    int shape;
} Food;

Ant *ants;
Food *foods;
void read_values_from_file(const char *filename) {
    FILE *file = fopen(filename, "r");
    if (file == NULL) {
        printf("Error opening file.\n");
        exit(1);
    }

    char buffer[MAX_BUFFER_SIZE];
    int line_number = 1;

    while (fgets(buffer, MAX_BUFFER_SIZE, file) != NULL) {
        char *name = strtok(buffer, ":");
        char *value_str = strtok(NULL, ":");
        if (name == NULL || value_str == NULL) {
            printf("Invalid line format in the file.\n");
            exit(1);
        }

        if (strcmp(name, "Ants_num") == 0) {
            Ants_num = atoi(value_str);
        } else if (strcmp(name, "Food_Pieces") == 0) {
            Food_Pieces = atoi(value_str);
        } else if (strcmp(name, "FOOD_INTERVAL") == 0) {
            FOOD_INTERVAL = atoi(value_str);
        } else if (strcmp(name, "FOOD_RADIUS") == 0) {
            FOOD_RADIUS = atoi(value_str);
        } else if (strcmp(name, "LIGHT_PHEROMONE_THRESHOLD") == 0) {
            LIGHT_PHEROMONE_THRESHOLD = atoi(value_str);
        } else if (strcmp(name, "STRONG_PHEROMONE_THRESHOLD") == 0) {
            STRONG_PHEROMONE_THRESHOLD = atoi(value_str);
        } else if (strcmp(name, "SIMULATION_TIME") == 0) {
            SIMULATION_TIME = atoi(value_str);
        } else {
            printf("Invalid parameter name in the file.\n");
            exit(1);
        }

        line_number++;
    }

    fclose(file);
}

void initialize_simulation()
{
    srand(time(NULL));
    // Place ants randomly on the screen
    printf("Placing ants...\n");
    for (int i = 0; i < Ants_num; i++)
    {
        ants[i].x = rand() % 380 - 190;
        ants[i].y = rand() % 180 - 90;
        ants[i].angle = rand() % 360; 
        ants[i].speed = 1 + rand() % 10;  // Speed range: 1 to 10
        ants[i].prevX = 9999;
        ants[i].prevY = 9999;
        ants[i].foodX = 9999;
        ants[i].foodY = 9999;
        ants[i].pheromone = 0;
        ants[i].seed = 4;
        printf("ant %d xposition: %lf ,yposition: %lf  ,angle: %lf ,speed: %d\n", i, ants[i].x,ants[i].y,ants[i].angle,ants[i].speed);
    }
}

// Function to calculate the angle between two points
double calculate_angle(double x1, double y1, double x2, double y2)
{
    double dx = x2 - x1;
    double dy = y2 - y1;
    double ang = atan2(dy, dx) * 180 / M_PI;
    return ang;
}

void ant_eating_food(int id, int foodid)
{
    if(foods[foodid].portion == 0) return;
    pthread_mutex_lock(&mutexes[foodid]);
    printf("ant %d eating food: %d \n", id, foodid);
    if(foods[foodid].portion > 0) 
        foods[foodid].portion -= 1;
    if(foods[foodid].portion == 0){
        printf("food: %d is gone\n", foodid);
        food_eaten += 1;
    }
    pthread_mutex_unlock(&mutexes[foodid]);
    sleep(1);
}

void ant_detects_food(int id)
{
    double minDistance = 9999;
    int flag = 0;
    for (int i = 0; i < food_num; i++)
    {   
        if(foods[i].portion == 0) continue;
        double distance = sqrt(pow(ants[id].x - foods[i].x, 2) + pow(ants[id].y - foods[i].y, 2));
       // printf("distance between ant %d and food: %d is %f \n", id, i, distance);
        if (distance <= 1.0)
        {
            ant_eating_food(id, i);
            return;
        }

        else if (distance <= FOOD_RADIUS && distance < minDistance)
        {
            printf("ant %d found food: %d \n", id, i);
            flag = 1;
            ants[id].foodX = foods[i].x;
            ants[id].foodY = foods[i].y;
            double newAngle = calculate_angle(ants[id].x, ants[id].y, foods[i].x, foods[i].y);
            ants[id].angle = newAngle;
            ants[id].pheromone = 3 ;
            ants[id].seed = foods[i].seed;
            minDistance = distance;
        } 
    }
    if(flag == 0){
        ants[id].foodX == 9999;  
        ants[id].foodY == 9999;
        ants[id].pheromone = 0;
        ants[id].seed = 4;
    }
    return;
}
void ant_detects_phermones(int id)
{
    double minDistance = 9999;
    int maxPhermone = 0;
    int shift =0;
    for (int i = 0; i < Ants_num; i++)
    {
        if (i != id)
        {
            double distance = sqrt(pow(ants[id].x - ants[i].x, 2) + pow(ants[id].y - ants[i].y, 2));
            //printf("distance between ant %d and ant %d is %f \n", id, i, distance);

            if (distance <= STRONG_PHEROMONE_THRESHOLD && distance < minDistance && ants[i].pheromone == 3 )
            {
                printf("******ant %d  is following ant %d  that found food*******\n", id, i);
                ants[id].foodX = ants[i].foodX;
                ants[id].foodY = ants[i].foodY;
                ants[id].seed = ants[i].seed;
                double newAngle = calculate_angle(ants[id].x, ants[id].y, ants[id].foodX, ants[id].foodY);
                ants[id].angle = newAngle;
                ants[id].pheromone = 2/ distance;
                minDistance = distance;
            }
            else if (distance <= LIGHT_PHEROMONE_THRESHOLD && ants[i].pheromone > maxPhermone && ants[id].foodX == 9999 )
            {
                int diffrence = ants[id].angle - ants[i].angle;
                if (diffrence > 0)
                {
                    shift =  -5;
                }
                else if (diffrence < 0)
                {
                    shift =  5;
                }
                ants[id].pheromone = 1/ distance;
                maxPhermone = ants[i].pheromone;
            }
        }
    }
    if(ants[id].foodX == 9999){
        if(shift != 0)
            printf("++++++ant %d  is moving by an angle of 5◦ in the direction of another ant++++++\n", id);

 ants[id].angle += shift;
    }

    return;
}

void *antMove(void *arg)
{
    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
    pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);

    int id = *(int *)arg;
    while (!isFinished)
    {

        // Simulate ant behavior
        // Update ant's position and direction based on random movements
        // and collision detection with screen boundaries

        //printf("Ant %d: Updating position and direction...\n", id);
        ant_detects_food(id);
        if (ants[id].foodX == 9999 || ants[id].foodY == 9999)
        {
            ant_detects_phermones(id);
            ants[id].seed = 4;
        }

        //if (ants[id].foodX != ants[id].x || ants[id].foodY != ants[id].y)
        //{
            double radians = ants[id].angle * M_PI / 180.0;
            ants[id].x += cos(radians) ;
            ants[id].y += sin(radians);
        
        if (ants[id].x < -190 || ants[id].x > 190 || ants[id].y < -90 || ants[id].y > 90) {
            int random_angle = (rand() % 2) == 0 ? 45 : -45;

        if (ants[id].x < -190)ants[id].x = -190;
        if (ants[id].x > 190) ants[id].x = 190;
        if (ants[id].y < -90)ants[id].y = -90;
        if (ants[id].y > 90) ants[id].y = 90;

        ants[id].angle += random_angle; // Change direction by 45 degrees
        if (ants[id].angle < 0)ants[id].angle += 360;
        if (ants[id].angle >= 360) ants[id].angle -= 360;

    }
        //}
        // Add collision detection with screen boundaries
        usleep((ants[id].speed ) * 2000); // Sleep for a short time to simulate time passing
        pthread_testcancel();

        
    }
        pthread_exit(NULL);

    // printf("Ants SIm");
    return NULL;
}

void *food_place(void *arg)
{
    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
    pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, NULL);
    while (!isFinished)
    {
        // Initialize food location

        foods[food_num].x = rand() % 360 - 180;
        foods[food_num].y = rand() % 160 - 80;
        foods[food_num].portion = 100;
        foods[food_num].seed = rand() % 3 + 1;
        foods[food_num].shape = rand() % 3 + 1;
        printf("Initializing food location...\n");
        printf("food %d x position: %lf and y position: %lf\n", food_num, foods[food_num].x,foods[food_num].y);
        food_num++;
        pthread_testcancel();

        sleep(FOOD_INTERVAL); // Wait for the defined interval before generating the next food
    }
        pthread_exit(NULL);

    return NULL;
}

void colorPicker(int value, int colorSeed){
    switch (colorSeed)
    {
    case 1:
        glColor3ub(1*value+30, 20*value+40, 20*value+50);
        break;
    case 2:
        glColor3ub(2*value+40, 20*value+30, 2*value+10);
        break;

    case 3:
        glColor3ub(20*value+20, 2*value+20, 5*value+10);
        break;

    case 4:
        glColor3ub(255, 255, 255);
        break;
    default:
        glColor3ub(0, 0, 0);
    }
    
}

void circle(float x, float y, int seed)
{
    int i;
    GLfloat radius;
    int triangleAmount =40;
    GLfloat twicePi = 2.0 * M_PI;
        glBegin(GL_TRIANGLE_FAN);
        
        radius =0.05;
        twicePi = 2.0 * M_PI;
        colorPicker(10,seed);
        glVertex2f(x, y); // center of circle
        for(i = 0; i <= triangleAmount;i++) {
        glVertex2f(
                    x + (radius * cos(i *  twicePi / triangleAmount)),
                    y + (radius * sin(i * twicePi / triangleAmount))
                  );
        }
    glEnd();
}

void polygon(float x, float y, int seed){
    glBegin(GL_POLYGON);
    colorPicker(10,seed);
    glVertex2f((GLfloat)x-0.03f,(GLfloat)y+0.03f);
    glVertex2f((GLfloat)x+0.03f,(GLfloat)y+0.03f);
    glVertex2f((GLfloat)x+0.06f,(GLfloat)y);
    glVertex2f((GLfloat)x+0.03f,(GLfloat)y-0.03f);
    glVertex2f((GLfloat)x-0.03f,(GLfloat)y-0.03f);
    glVertex2f((GLfloat)x-0.06f,(GLfloat)y);
    glEnd();
}

void triangle(float x, float y, int seed){

    glBegin(GL_TRIANGLES);
	colorPicker(10,seed);
	glVertex2f((GLfloat)x,(GLfloat)y+0.09f);
	glVertex2f((GLfloat)x+0.09f,(GLfloat)y);
	glVertex2f((GLfloat)x+0.09f,(GLfloat)y+0.09f);

	glEnd();

	glFlush();
}

void clearAnts(){
     int i;
    for(i = 0 ; i < Ants_num; i++){//x
        glPointSize(5.0f);
        glBegin(GL_POINTS);
            glColor3f(0.0f, 0.0f, 0.0f); // Whited
        glVertex2f((float)ants[i].x*0.01,(float)ants[i].y*0.01); //x,y
        glEnd();
    }
    return;
}

void clearFood(){
     int i;
    for(i = 0 ; i < Ants_num; i++){//x
       circle(foods[i].x*0.01,foods[i].y*0.01,0); 
    }
    return;
}

void addAnts() {
    clearAnts();
    int i;
    for(i = 0 ; i < Ants_num; i++){//x
        glPointSize(5.0f);
        glBegin(GL_POINTS);
        colorPicker(10,ants[i].seed);
        glVertex2f((float)ants[i].x*0.01,(float)ants[i].y*0.01); //x,y
        glEnd();
    }
    return;
}

void addFood() {

    clearFood();
    int i;
    for(i = 0 ; i < food_num; i++){//x
        if(foods[i].portion == 0) continue;

        switch (foods[i].shape)
        {
        case 1:
            circle(foods[i].x*0.01,foods[i].y*0.01,foods[i].seed);
            break;
        case 2:
            polygon(foods[i].x*0.01,foods[i].y*0.01,foods[i].seed);
            break;
        case 3:
            triangle(foods[i].x*0.01,foods[i].y*0.01,foods[i].seed);
        default:
            break;
        }
        
    }
}

void drawPageFrame() {  //here  put these inside the drawPageFrame(float progress1, float progress2){
	//and insted of 0.4 write the variables progress1 and the second 0.4 replace it with progress2 
    // Set the color of the frame to blue
    glColor3f(0.0, 0.0, 1.0);

    // Draw the top and bottom sides of the frame
    glBegin(GL_QUADS);
        glVertex2f(-2.0, 1.0);
        glVertex2f(2.0, 1.0);
        glVertex2f(2.0, 0.97);
        glVertex2f(-2.0, 0.97);

        glVertex2f(-2.0, -1.0);
        glVertex2f(2.0, -1.0);
        glVertex2f(2.0, -0.97);
        glVertex2f(-2.0, -0.97);
    glEnd();

    // Draw the left and right sides of the frame
    glBegin(GL_QUADS);
        glVertex2f(-2.0, 1.0);
        glVertex2f(-1.97, 1.0);
        glVertex2f(-1.97, -1.0);
        glVertex2f(-2.0, -1.0);

        glVertex2f(2.0, 1.0);
        glVertex2f(1.97, 1.0);
        glVertex2f(1.97, -1.0);
        glVertex2f(2.0, -1.0);
    glEnd();

    addAnts();
    glFlush();
    addFood();
    glFlush();
}

void display() {

    glClear(GL_COLOR_BUFFER_BIT);

    // Draw the page frame
    drawPageFrame();
    // Swap the buffers to update the display
    glutSwapBuffers();
    sleep(1);
    glutPostRedisplay();
    // Clear the window

}

void timer(int value) {
    int framesPerSecond = 120;
    glutPostRedisplay();  // Request a redraw
    glutTimerFunc(1000 / framesPerSecond, timer, 0);  // Set the timer callback function
}

/* Handler for window re-size event. Called back when the window first appears and
   whenever the window is re-sized with its new width and height */
void reshape(GLsizei width, GLsizei height) {  // GLsizei for non-negative integer
   // Compute aspect ratio of the new window
   if (height == 0) height = 1;                // To prevent divide by 0
   GLfloat aspect = (GLfloat)width / (GLfloat)height;
 
   // Set the viewport to cover the new window
   glViewport(0, 0, width, height);
 
   // Set the aspect ratio of the clipping area to match the viewport
   glMatrixMode(GL_PROJECTION);  // To operate on the Projection matrix
   glLoadIdentity();
   if (width >= height) {
     // aspect >= 1, set the height from -1 to 1, with larger width
      gluOrtho2D(-1.0 * aspect, 1.0 * aspect, -1.0, 1.0);
   } else {
      // aspect < 1, set the width to -1 to 1, with larger height
     gluOrtho2D(-1.0, 1.0, -1.0 / aspect, 1.0 / aspect);
   }
}
void *disply_opengl(void *arg){
   // Initialize GLUT

    // Set the window size and position
   
    glutInitWindowSize(1000, 500);
    glutInitWindowPosition(0, 0);
  

    // Set the display mode to double buffering
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);

    // Create the window
    glutCreateWindow("Ant Colony");
    // Set the display callback function
    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutTimerFunc(0, timer, 0);

    // Enter the GLUT event loop
    glutMainLoop();
return;

}


int main(int argc, char** argv)
{
     const char *filename = "param.txt";

    read_values_from_file(filename);
     ants = (Ant *)malloc(Ants_num * sizeof(Ant));
     foods = (Food *)malloc(Food_Pieces * sizeof(Food));
    mutexes = (pthread_mutex_t *)malloc(Food_Pieces * sizeof(pthread_mutex_t));

    pthread_t ant_threads[Ants_num];
    pthread_t foodAdd_thread;
    pthread_t opengl;

    initialize_simulation();
    glutInit(&argc, argv);

    // Create threads for ants
    for (int i = 0; i < Ants_num; i++)
    {
        int *id = malloc(sizeof(int));
        *id = i;
        pthread_create(&ant_threads[i], NULL, antMove, id);
    }
    for (int i = 0; i < Food_Pieces; i++) {
        pthread_mutex_init(&mutexes[i], NULL);
    }
    pthread_create(&foodAdd_thread, NULL, food_place, NULL);
    pthread_create(&opengl, NULL, disply_opengl, NULL);


 
   
   sleep(SIMULATION_TIME);
    isFinished = 1;

    // Terminate the threads
    for (int i = 0; i < Ants_num; i++) {
        pthread_cancel(&ants[i]);
    }

    pthread_cancel(foodAdd_thread);
    pthread_cancel(opengl);

    /*
    // Wait for ant threads to finish 
    for (int i = 0; i < Ants_num; i++)
    {
        pthread_join(ant_threads[i], NULL);
    }
    pthread_join(foodAdd_thread, NULL);
*/
    printf("SIMULATION Done , Number of Food Eaten : %d\n",food_eaten);
    return 0;
}