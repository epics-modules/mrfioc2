#include <vector>
#include <algorithm>

#include "epicsUnitTest.h"
#include "epicsString.h"
#include "testMain.h"

#include "mrf/object.h"

namespace {
using namespace mrf;

class mine : public ObjectInst<mine>
{
public:
    int ival;
    double dval;
    std::vector<double> darr;
    unsigned count;

    mine(const std::string& n) : ObjectInst<mine>(n), ival(0), dval(0.0), count(0)
    {}

    /* no locking needed */
    virtual void lock() const{};
    virtual void unlock() const{};

    int getI() const{return ival;}
    void setI(int i){ival=i;}

    double val() const{return dval;}
    void setVal(double v){dval=v;}

    epicsUInt32 getdarr(double* v, epicsUInt32 l) const
    {
		if (!v) return (epicsUInt32)darr.size();
		l = (epicsUInt32)std::min((size_t)l, darr.size());
        std::copy(darr.begin(), darr.begin()+l, v);
		return l;
    }
    void setdarr(const double* v, epicsUInt32 l)
    {
        darr.resize(l);
        std::copy(v, v+l, darr.begin());
    }

    void incr() { count++; }
};

class other : public ObjectInst<other, mine>
{
    typedef ObjectInst<other, mine> base_t;
public:
    other(const std::string& n) : base_t(n) {}
    virtual ~other() {}

    int getX() const { return 42;}

    static Object* buildOne(const std::string& name, const std::string& klass, const Object::create_args_t& args);
};

Object*
other::buildOne(const std::string& name, const std::string& klass, const Object::create_args_t& args)
{
    return new other(name);
}


void testMine()
{
    testDiag("In testMine()");
    mine m("test");

    testOk1(m.getI()==0);

    mrf::auto_ptr<property<int> > I=m.getProperty<int>("I");
    testOk1(I.get()!=NULL);

    if(I.get()) {
        testOk1(I->get()==0);
        I->set(42);
        testOk1(m.ival==42);
    }

    Object *o = &m;

    mrf::auto_ptr<property<double> > V=o->getProperty<double>("val");
    testOk1(V.get()!=NULL);

    if(V.get()) {
        testOk1(V->get()==0.0);
        V->set(4.2);
        testOk1(m.dval==4.2);
    }

    mrf::auto_ptr<property<int> > I2=o->getProperty<int>("I");
    testOk1(I2.get()!=NULL);
    testOk1((*I)==(*I2));

    if(I2.get())
        testOk1(I2->get()==42);

    I2=o->getProperty<int>("val");
    testOk1(I2.get()!=NULL);
    testOk1((*I)!=(*I2));

    if(I2.get())
        testOk1(I2->get()==42);

    mrf::auto_ptr<property<double[1]> > A=o->getProperty<double[1]>("darr");
    testOk1(A.get()!=NULL);

    const double tst[] = {1.0, 2.0, 3.0};
    double tst2[3];

    if(A.get()) {
        A->set(tst,3);
        testOk1(m.darr.size()==3);
        testOk1(A->get(tst2,3)==3);
    }

    testOk1(std::equal(tst,tst+3,tst2));

    V=o->getProperty<double>("other");
    testOk1(V.get()==NULL);

    try {
        mine n("test"); // duplicate name
        testFail("Duplicate name not prevented");
    } catch(std::runtime_error& e) {
        testPass("Duplicate name prevented: %s",e.what());
    }

    Object *p=Object::getObject("test");
    testOk1(p!=NULL);
    testOk1(p==o);

    {
        testOk1(m.count==0);
        mrf::auto_ptr<property<void> > incr(o->getProperty<void>("incr"));
        testOk1(incr.get()!=NULL);
        if(incr.get()) {
            incr->exec();
        }
        testOk1(m.count==1);
    }
}

void testOther()
{
    testDiag("In testOther()");
    other m("foo");

    mrf::auto_ptr<property<double> > V=m.getProperty<double>("val");
    testOk1(V.get()!=NULL);

    mrf::auto_ptr<property<int> > I=m.getProperty<int>("I");
    testOk1(I.get()!=NULL);

    mrf::auto_ptr<property<double[1]> > A=m.getProperty<double[1]>("darr");
    testOk1(A.get()!=NULL);

    mrf::auto_ptr<property<int> > X=m.getProperty<int>("X");
    testOk1(X.get()!=NULL);

    if(X.get())
        testOk1(X->get()==42);
    else
        testSkip(1, "NULL");
}

void testOther2()
{
    testDiag("In testOther2()");
    other m("foo");
    Object *o = &m;

    mrf::auto_ptr<property<double> > V=o->getProperty<double>("val");
    testOk1(V.get()!=NULL);

    mrf::auto_ptr<property<int> > I=o->getProperty<int>("I");
    testOk1(I.get()!=NULL);

    mrf::auto_ptr<property<double[1]> > A=o->getProperty<double[1]>("darr");
    testOk1(A.get()!=NULL);

    mrf::auto_ptr<property<int> > X=o->getProperty<int>("X");
    testOk1(X.get()!=NULL);
}

void testFactory()
{
    testDiag("In testFactory()");
    other m("HelloWorld");

    testOk1(Object::getObject("NoOnesHome")==NULL);
    testOk1(Object::getObject("HelloWorld")==&m);

    testOk1(Object::getCreateObject("HelloWorld", "other")==&m);

    Object *built = Object::getCreateObject("AnotherOne", "other");
    testOk1(built!=NULL);

    testOk1(built==Object::getObject("AnotherOne"));
    testOk1(built==Object::getCreateObject("AnotherOne", "other"));
}

} // namespace

OBJECT_BEGIN(mine)
OBJECT_PROP2("I",   &mine::getI,    &mine::setI);
OBJECT_PROP2("val", &mine::getI,    &mine::setI);
OBJECT_PROP2("val", &mine::val,     &mine::setVal);
OBJECT_PROP2("darr",&mine::getdarr, &mine::setdarr);
OBJECT_PROP1("incr", &mine::incr);
OBJECT_END(mine)

OBJECT_BEGIN2(other, mine)
OBJECT_PROP1("X", &other::getX);
OBJECT_FACTORY(other::buildOne);
OBJECT_END(other)

MAIN(objectTest)
{
    testPlan(39);
    testMine();
    testOther();
    testOther2();
    testFactory();
    return testDone();
}
